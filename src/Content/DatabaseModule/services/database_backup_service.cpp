#include "Content/DatabaseModule/services/database_backup_service.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"
#include "Libs/Resources/resources.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <utility>

namespace {

constexpr const char* kBackupDirectoryName = "backups";
constexpr const char* kBackupExtension = ".db";
constexpr const char* kBeforeRestoreSuffix = "-before-restore";
constexpr int kDefaultWalInterval = 60000;
constexpr int kDefaultWalMaxSizeMb = 20;

void assignError(QString* error, const QString& message) {
	if (error) {
		*error = message;
	}
}

bool validAlias(const QString& alias) {
	static const QRegularExpression expression(QStringLiteral("^[a-z0-9_-]+$"));
	return expression.match(alias).hasMatch();
}

QString timestampFor(const QDateTime& time) {
	return time.toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
}

bool positiveValue(const QVariant& value, int fallback) {
	bool ok = false;
	const int result = value.toInt(&ok);
	return ok && result > 0 ? result : fallback;
}

/// Скопировать файл и убедиться, что размер копии совпадает с исходником.
bool copyFile(const QString& source, const QString& target) {
	if (!QFile::copy(source, target)) {
		return false;
	}

	return QFileInfo(target).size() == QFileInfo(source).size();
}

/// Убрать журналы SQLite: после замены файла старые WAL/SHM недопустимы.
void removeSidecars(const QString& databasePath) {
	for (const auto& suffix : { QStringLiteral("-wal"), QStringLiteral("-shm"), QStringLiteral("-journal") }) {
		QFile::remove(databasePath + suffix);
	}
}

bool isInsideDirectory(const QString& directory, const QString& path) {
	const auto relative = QDir::cleanPath(QDir(directory).relativeFilePath(path));
	return relative != QStringLiteral("..")
		&& !relative.startsWith(QStringLiteral("../"))
		&& !QDir::isAbsolutePath(relative);
}

} // namespace

class DatabaseBackupService::Private {
public:
	Private(DatabaseBackupService* parent)
		: q(parent) {
	}

	DatabaseBackupService* q = nullptr;
	Resources* resources = nullptr;
	std::shared_ptr<CommandContext> globalContext;
	DatabasesService* databases = nullptr;

	std::shared_ptr<CommandContext> storeContext(const QString& alias) const {
		if (!globalContext || !validAlias(alias)) {
			return nullptr;
		}

		return globalContext->context(QStringLiteral(".store.%1").arg(alias));
	}

	QString backupsRoot() const {
		if (!resources) {
			return {};
		}

		const auto profile = resources->Directories.get(QStringLiteral("path.profile"));
		if (!profile) {
			return {};
		}

		return profile->filePath(QString::fromLatin1(kBackupDirectoryName));
	}

	QList<DatabaseBackupInfo> readBackups(const QString& alias) const;
	std::optional<DatabaseStoreInfo> storeInfo(const QString& alias) const;
};

QList<DatabaseBackupInfo> DatabaseBackupService::Private::readBackups(const QString& alias) const {
	QList<DatabaseBackupInfo> result;
	const auto directory = q->backupDirectory(alias);
	if (directory.isEmpty()) {
		return result;
	}

	QDir dir(directory);
	if (!dir.exists()) {
		return result;
	}

	const auto entries = dir.entryInfoList({ QStringLiteral("*") + QString::fromLatin1(kBackupExtension) },
		QDir::Files | QDir::Readable, QDir::Time);
	for (const auto& entry : entries) {
		DatabaseBackupInfo info;
		info.alias = alias;
		info.path = entry.absoluteFilePath();
		info.createdAt = entry.lastModified();
		info.size = entry.size();
		info.automatic = entry.completeBaseName().endsWith(QString::fromLatin1(kBeforeRestoreSuffix));
		result.append(info);
	}

	return result;
}

std::optional<DatabaseStoreInfo> DatabaseBackupService::Private::storeInfo(const QString& alias) const {
	if (!databases || !validAlias(alias)) {
		return std::nullopt;
	}

	const auto context = storeContext(alias);
	if (!context) {
		return std::nullopt;
	}

	DatabaseStoreInfo info;
	info.alias = alias;
	info.path = databases->databasePath(alias);
	if (info.path.isEmpty()) {
		return std::nullopt;
	}

	info.encrypted = context->variable(QStringLiteral("encrypted"), false).toBool();
	info.walCheckInterval = positiveValue(
		context->variable(QStringLiteral("wal.checkInterval"), kDefaultWalInterval),
		kDefaultWalInterval);
	info.walMaxSizeMb = positiveValue(
		context->variable(QStringLiteral("wal.maxSizeMb"), kDefaultWalMaxSizeMb),
		kDefaultWalMaxSizeMb);

	const QFileInfo file(info.path);
	info.fileExists = file.exists() && file.isFile();
	info.fileSize = info.fileExists ? file.size() : 0;

	const auto backups = readBackups(alias);
	info.backupCount = backups.size();
	if (!backups.isEmpty()) {
		info.lastBackupAt = backups.first().createdAt;
	}

	return info;
}

DatabaseBackupService::DatabaseBackupService(Resources* resources,
	std::shared_ptr<CommandContext> globalContext, DatabasesService* databases, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
	d->globalContext = std::move(globalContext);
	d->databases = databases;
}

DatabaseBackupService::~DatabaseBackupService() = default;

QString DatabaseBackupService::backupDirectory(const QString& alias) const {
	const auto normalized = alias.trimmed().toLower();
	if (!validAlias(normalized)) {
		return {};
	}

	const auto root = d->backupsRoot();
	if (root.isEmpty()) {
		return {};
	}

	return QDir(root).filePath(normalized);
}

QList<DatabaseStoreInfo> DatabaseBackupService::stores() const {
	QList<DatabaseStoreInfo> result;
	const auto storesContext = d->globalContext
		? d->globalContext->context(QStringLiteral(".store"))
		: nullptr;
	if (!storesContext) {
		return result;
	}

	for (const auto& context : storesContext->childContexts()) {
		if (!context) {
			continue;
		}
		if (auto info = d->storeInfo(context->name())) {
			result.append(*info);
		}
	}

	std::sort(result.begin(), result.end(), [](const DatabaseStoreInfo& left, const DatabaseStoreInfo& right) {
		return left.alias < right.alias;
	});
	return result;
}

std::optional<DatabaseStoreInfo> DatabaseBackupService::store(const QString& alias) const {
	return d->storeInfo(alias.trimmed().toLower());
}

QList<DatabaseBackupInfo> DatabaseBackupService::backups(const QString& alias) const {
	const auto normalized = alias.trimmed().toLower();
	if (!d->storeInfo(normalized)) {
		return {};
	}

	return d->readBackups(normalized);
}

std::optional<DatabaseBackupInfo> DatabaseBackupService::createBackup(const QString& alias,
	QString* error) {
	assignError(error, QString());
	const auto normalized = alias.trimmed().toLower();
	const auto info = d->storeInfo(normalized);
	if (!info) {
		assignError(error, tr("Хранилище «%1» не зарегистрировано в контексте .store").arg(alias));
		return std::nullopt;
	}
	if (!d->databases) {
		assignError(error, tr("Сервис баз данных недоступен"));
		return std::nullopt;
	}

	// Открытие гарантирует, что файл существует и миграции применены.
	auto* connection = d->databases->connection(normalized);
	if (!connection) {
		assignError(error, tr("Не удалось открыть базу «%1»: проверьте ключ шифрования и миграции")
			.arg(normalized));
		return std::nullopt;
	}

	// WAL свёрнут в основной файл: копия файла соответствует последнему коммиту.
	auto reader = connection->executeQuery(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"));
	const bool checkpointed = reader && reader->next() && reader->value(0).toInt() == 0;
	reader.reset();
	if (!checkpointed) {
		assignError(error, tr("Не удалось завершить WAL-сессию базы «%1»: файл занят").arg(normalized));
		return std::nullopt;
	}

	const auto directory = backupDirectory(normalized);
	if (directory.isEmpty() || !QDir().mkpath(directory)) {
		assignError(error, tr("Не удалось создать каталог резервных копий «%1»").arg(directory));
		return std::nullopt;
	}

	const auto createdAt = QDateTime::currentDateTime();
	const auto baseName = timestampFor(createdAt);
	QString target = QDir(directory).filePath(baseName + QString::fromLatin1(kBackupExtension));
	int suffix = 1;
	while (QFile::exists(target)) {
		target = QDir(directory).filePath(QStringLiteral("%1-%2%3")
			.arg(baseName).arg(suffix++).arg(QString::fromLatin1(kBackupExtension)));
	}

	if (!copyFile(info->path, target)) {
		QFile::remove(target);
		assignError(error, tr("Не удалось скопировать файл базы «%1»").arg(info->path));
		return std::nullopt;
	}

	DatabaseBackupInfo created;
	created.alias = normalized;
	created.path = target;
	created.createdAt = createdAt;
	created.size = QFileInfo(target).size();

	emit backupCreated(normalized, target);
	emit backupsChanged(normalized);
	return created;
}

bool DatabaseBackupService::restoreBackup(const QString& alias, const QString& backupPath,
	QString* error) {
	assignError(error, QString());
	const auto normalized = alias.trimmed().toLower();
	const auto info = d->storeInfo(normalized);
	if (!info) {
		assignError(error, tr("Хранилище «%1» не зарегистрировано в контексте .store").arg(alias));
		return false;
	}
	if (!d->databases) {
		assignError(error, tr("Сервис баз данных недоступен"));
		return false;
	}

	const QFileInfo source(backupPath);
	if (!source.exists() || !source.isFile() || source.size() <= 0) {
		assignError(error, tr("Файл копии не найден или пуст: %1").arg(backupPath));
		return false;
	}
	if (source.absoluteFilePath().compare(info->path, Qt::CaseInsensitive) == 0) {
		assignError(error, tr("Файл копии совпадает с файлом базы «%1»").arg(normalized));
		return false;
	}

	// Проверяем копию до любых изменений рабочей базы.
	QString verifyError;
	if (!d->databases->verifyDatabaseFile(normalized, source.absoluteFilePath(), &verifyError)) {
		assignError(error, tr("Копия не является корректной базой «%1»: %2")
			.arg(normalized, verifyError));
		return false;
	}

	const auto targetPath = info->path;
	d->databases->closeDatabase(normalized);

	// Страховочная копия текущего файла: её делает только что закрытое
	// соединение, то есть WAL уже свёрнут.
	QString safetyPath;
	if (QFile::exists(targetPath)) {
		const auto directory = backupDirectory(normalized);
		if (directory.isEmpty() || !QDir().mkpath(directory)) {
			assignError(error, tr("Не удалось создать каталог страховочной копии для «%1»")
				.arg(normalized));
			d->databases->connection(normalized);
			return false;
		}

		const auto baseName = timestampFor(QDateTime::currentDateTime())
			+ QString::fromLatin1(kBeforeRestoreSuffix);
		safetyPath = QDir(directory).filePath(baseName + QString::fromLatin1(kBackupExtension));
		int suffix = 1;
		while (QFile::exists(safetyPath)) {
			safetyPath = QDir(directory).filePath(QStringLiteral("%1-%2%3")
				.arg(baseName).arg(suffix++).arg(QString::fromLatin1(kBackupExtension)));
		}
		if (!copyFile(targetPath, safetyPath)) {
			assignError(error, tr("Не удалось сохранить страховочную копию базы «%1»")
				.arg(normalized));
			d->databases->connection(normalized);
			return false;
		}
	}

	removeSidecars(targetPath);
	if (QFile::exists(targetPath) && !QFile::remove(targetPath)) {
		assignError(error, tr("Не удалось удалить текущий файл базы «%1»").arg(targetPath));
		// Соединение уже закрыто — возвращаем приложение к рабочей базе.
		d->databases->connection(normalized);
		return false;
	}

	if (!copyFile(source.absoluteFilePath(), targetPath)) {
		QFile::remove(targetPath);
		if (!safetyPath.isEmpty()) {
			copyFile(safetyPath, targetPath);
		}
		d->databases->connection(normalized);
		assignError(error, tr("Не удалось скопировать копию на место базы «%1»").arg(normalized));
		return false;
	}

	if (!d->databases->connection(normalized)) {
		// Восстановленная база не открывается: возвращаем прежний файл.
		QFile::remove(targetPath);
		removeSidecars(targetPath);
		if (!safetyPath.isEmpty()) {
			copyFile(safetyPath, targetPath);
		}
		d->databases->connection(normalized);
		assignError(error, tr("База «%1» восстановлена, но не открывается "
			"(проверьте ключ шифрования и миграции). Изменения отменены.").arg(normalized));
		return false;
	}

	emit backupsChanged(normalized);
	emit backupRestored(normalized, source.absoluteFilePath());
	return true;
}

bool DatabaseBackupService::removeBackup(const QString& alias, const QString& backupPath,
	QString* error) {
	assignError(error, QString());
	const auto normalized = alias.trimmed().toLower();
	if (!d->storeInfo(normalized)) {
		assignError(error, tr("Хранилище «%1» не зарегистрировано в контексте .store").arg(alias));
		return false;
	}

	const QFileInfo file(backupPath);
	if (!file.exists() || !file.isFile()) {
		assignError(error, tr("Файл копии не найден: %1").arg(backupPath));
		return false;
	}

	const auto directory = backupDirectory(normalized);
	if (directory.isEmpty() || !isInsideDirectory(directory, file.absoluteFilePath())) {
		assignError(error, tr("Файл не принадлежит каталогу копий хранилища «%1»").arg(normalized));
		return false;
	}

	if (!QFile::remove(file.absoluteFilePath())) {
		assignError(error, tr("Не удалось удалить файл копии «%1»").arg(file.absoluteFilePath()));
		return false;
	}

	emit backupsChanged(normalized);
	return true;
}
