#include "Content/DatabaseModule/documents/database_backup_document.h"
#include "Content/DatabaseModule/services/database_backup_service.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/DatabaseModule/widgets/database_backup_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"
#include "Libs/Resources/resources.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <memory>
#include <vector>

namespace {

class TestController : public CommandController {
public:
	explicit TestController(Resources* resources)
		: CommandController(resources) {
	}

	CommandProcessor* commandProcessor() const override {
		return nullptr;
	}

	ServicesRegistry* services() const override {
		return nullptr;
	}

private:
	void initContext() override {
	}
};

/// Временный профиль и собранные на нём сервисы модуля.
struct TestEnvironment {
	QTemporaryDir dir;
	Resources resources;
	std::unique_ptr<TestController> controller;
	std::unique_ptr<DatabasesService> databases;
	std::unique_ptr<DatabaseBackupService> backup;
	/// Контексты `.store` и `.store.<alias>`: дерево хранит только слабые
	/// ссылки, поэтому тест обязан держать владение (как BootstrapContextBuilder).
	std::shared_ptr<CommandContext> storesContext;
	std::vector<std::shared_ptr<CommandContext>> storeContexts;

	TestEnvironment() {
		resources.Directories.set("path.profile", dir.path());
		resources.Directories.set("path.databases",
			QDir(dir.path()).filePath(QStringLiteral("data")));
		controller = std::make_unique<TestController>(&resources);
		databases = std::make_unique<DatabasesService>(&resources, controller->globalContext());
		backup = std::make_unique<DatabaseBackupService>(&resources,
			controller->globalContext(), databases.get());
	}

	std::shared_ptr<CommandContext> addStore(const QString& alias, bool encrypted = false) {
		if (!storesContext) {
			storesContext = controller->globalContext()->context(QStringLiteral(".store"));
		}
		if (!storesContext) {
			storesContext = controller->globalContext()->createScopedContext(QStringLiteral("store"));
		}

		auto context = storesContext->createScopedContext(alias);
		context->setVariable(QStringLiteral("encrypted"), encrypted);
		storeContexts.push_back(context);
		return context;
	}
};

bool startsAsSqlite(const QString& path) {
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return false;
	}

	return file.read(16) == QByteArray("SQLite format 3\0", 16);
}

QString singleValue(SQLiteConnection* connection, const QString& table) {
	if (!connection) {
		return {};
	}

	auto reader = connection->executeQuery(QStringLiteral("SELECT value FROM %1").arg(table));
	if (!reader || !reader->next()) {
		return {};
	}

	return reader->value(0).toString();
}

} // namespace

class TestDatabaseBackup : public QObject {
	Q_OBJECT
private slots:
	void storesComeFromStoreContexts();
	void defaultProfileHidesEncryptedStores();
	void createsBackupAndListsIt();
	void restoreReplacesDatabaseAndKeepsSafetyCopy();
	void encryptedBackupStaysEncrypted();
	void restoreRejectsBrokenBackup();
	void removeBackupOnlyInsideBackupDirectory();
	void widgetAndDocumentShareSelectedStore();
	void deleteButtonIsRightAlignedAndRed();
};

void TestDatabaseBackup::storesComeFromStoreContexts() {
	TestEnvironment environment;
	environment.addStore(QStringLiteral("beta"));
	environment.addStore(QStringLiteral("alpha"));

	const auto stores = environment.backup->stores();
	QCOMPARE(stores.size(), 2);
	QCOMPARE(stores.at(0).alias, QStringLiteral("alpha"));
	QCOMPARE(stores.at(1).alias, QStringLiteral("beta"));
	QVERIFY(stores.at(0).path.endsWith(QStringLiteral("alpha.db")));
	QVERIFY(!stores.at(0).fileExists);
	QCOMPARE(stores.at(0).backupCount, 0);
	QVERIFY(!environment.backup->store(QStringLiteral("unknown")).has_value());

	const auto directory = environment.backup->backupDirectory(QStringLiteral("alpha"));
	QVERIFY(directory.endsWith(QStringLiteral("backups/alpha"))
		|| directory.endsWith(QStringLiteral("backups\\alpha")));
}

void TestDatabaseBackup::defaultProfileHidesEncryptedStores() {
	TestEnvironment environment;
	environment.resources.Directories.set(QStringLiteral("path.profiles"),
		QDir(environment.dir.path()).filePath(QStringLiteral("profiles")));
	environment.addStore(QStringLiteral("alpha"));
	environment.addStore(QStringLiteral("secret"), true);

	// Профиль неавторизованного пользователя: только открытые хранилища.
	QVERIFY(environment.resources.selectProfile(QStringLiteral("default")));
	const auto restricted = environment.backup->stores();
	QCOMPARE(restricted.size(), 1);
	QCOMPARE(restricted.first().alias, QStringLiteral("alpha"));
	QVERIFY(!environment.backup->store(QStringLiteral("secret")).has_value());

	// После входа зашифрованное хранилище снова доступно.
	QVERIFY(environment.resources.selectProfile(QStringLiteral("abc123")));
	QCOMPARE(environment.backup->stores().size(), 2);
	QVERIFY(environment.backup->store(QStringLiteral("secret")).has_value());
}

void TestDatabaseBackup::createsBackupAndListsIt() {
	TestEnvironment environment;
	environment.addStore(QStringLiteral("alpha"));

	auto* database = environment.databases->connection(QStringLiteral("alpha"));
	QVERIFY(database);
	QVERIFY(database->execute(QStringLiteral(
		"CREATE TABLE sample (value TEXT); INSERT INTO sample VALUES ('one');")));

	QSignalSpy changed(environment.backup.get(), &DatabaseBackupService::backupsChanged);
	QString error;
	const auto created = environment.backup->createBackup(QStringLiteral("alpha"), &error);
	QVERIFY2(created.has_value(), qPrintable(error));
	QVERIFY(QFile::exists(created->path));
	QCOMPARE(created->size, QFileInfo(environment.databases->databasePath(
		QStringLiteral("alpha"))).size());
	QCOMPARE(changed.count(), 1);

	const auto backups = environment.backup->backups(QStringLiteral("alpha"));
	QCOMPARE(backups.size(), 1);
	QCOMPARE(backups.first().path, created->path);
	QVERIFY(!backups.first().automatic);
	QCOMPARE(environment.backup->store(QStringLiteral("alpha"))->backupCount, 1);

	// Копия — самостоятельная база с теми же данными.
	QVERIFY(startsAsSqlite(created->path));
	SQLiteConnection copy;
	QVERIFY(copy.open(QStringLiteral("file=%1;").arg(created->path)));
	QCOMPARE(singleValue(&copy, QStringLiteral("sample")), QStringLiteral("one"));
}

void TestDatabaseBackup::restoreReplacesDatabaseAndKeepsSafetyCopy() {
	TestEnvironment environment;
	environment.addStore(QStringLiteral("alpha"));

	auto* database = environment.databases->connection(QStringLiteral("alpha"));
	QVERIFY(database);
	QVERIFY(database->execute(QStringLiteral(
		"CREATE TABLE sample (value TEXT); INSERT INTO sample VALUES ('first');")));

	QString error;
	const auto created = environment.backup->createBackup(QStringLiteral("alpha"), &error);
	QVERIFY2(created.has_value(), qPrintable(error));

	auto* open = environment.databases->connection(QStringLiteral("alpha"));
	QVERIFY(open);
	QVERIFY(open->execute(QStringLiteral("UPDATE sample SET value = 'second';")));
	QCOMPARE(singleValue(open, QStringLiteral("sample")), QStringLiteral("second"));

	QVERIFY2(environment.backup->restoreBackup(QStringLiteral("alpha"), created->path, &error),
		qPrintable(error));
	QCOMPARE(singleValue(environment.databases->connection(QStringLiteral("alpha")),
		QStringLiteral("sample")), QStringLiteral("first"));

	const auto backups = environment.backup->backups(QStringLiteral("alpha"));
	QCOMPARE(backups.size(), 2);
	const auto automatic = std::count_if(backups.cbegin(), backups.cend(),
		[](const DatabaseBackupInfo& info) { return info.automatic; });
	QCOMPARE(automatic, 1);
}

void TestDatabaseBackup::encryptedBackupStaysEncrypted() {
	TestEnvironment environment;
	environment.addStore(QStringLiteral("secret"), true);
	environment.databases->setEncryptionKey(QByteArray(32, 'k'));

	auto* database = environment.databases->connection(QStringLiteral("secret"));
	QVERIFY(database);
	QVERIFY(database->execute(QStringLiteral(
		"CREATE TABLE secrets (value TEXT); INSERT INTO secrets VALUES ('hidden');")));

	QString error;
	const auto created = environment.backup->createBackup(QStringLiteral("secret"), &error);
	QVERIFY2(created.has_value(), qPrintable(error));
	QVERIFY(!startsAsSqlite(created->path));

	QVERIFY(database->execute(QStringLiteral("UPDATE secrets SET value = 'changed';")));
	QVERIFY2(environment.backup->restoreBackup(QStringLiteral("secret"), created->path, &error),
		qPrintable(error));
	QCOMPARE(singleValue(environment.databases->connection(QStringLiteral("secret")),
		QStringLiteral("secrets")), QStringLiteral("hidden"));

	// Без ключа копию нельзя ни проверить, ни восстановить.
	environment.databases->cleadEncryptionKey();
	error.clear();
	QVERIFY(!environment.backup->restoreBackup(QStringLiteral("secret"), created->path, &error));
	QVERIFY(!error.isEmpty());
}

void TestDatabaseBackup::restoreRejectsBrokenBackup() {
	TestEnvironment environment;
	environment.addStore(QStringLiteral("alpha"));

	auto* database = environment.databases->connection(QStringLiteral("alpha"));
	QVERIFY(database);
	QVERIFY(database->execute(QStringLiteral(
		"CREATE TABLE sample (value TEXT); INSERT INTO sample VALUES ('intact');")));

	const auto broken = QDir(environment.dir.path()).filePath(QStringLiteral("broken.db"));
	QFile file(broken);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write("this is not a database");
	file.close();

	QString error;
	QVERIFY(!environment.backup->restoreBackup(QStringLiteral("alpha"), broken, &error));
	QVERIFY(!error.isEmpty());
	// Рабочая база не тронута.
	QCOMPARE(singleValue(environment.databases->connection(QStringLiteral("alpha")),
		QStringLiteral("sample")), QStringLiteral("intact"));
}

void TestDatabaseBackup::removeBackupOnlyInsideBackupDirectory() {
	TestEnvironment environment;
	environment.addStore(QStringLiteral("alpha"));
	QVERIFY(environment.databases->connection(QStringLiteral("alpha")));

	QString error;
	const auto created = environment.backup->createBackup(QStringLiteral("alpha"), &error);
	QVERIFY2(created.has_value(), qPrintable(error));

	QVERIFY2(environment.backup->removeBackup(QStringLiteral("alpha"), created->path, &error),
		qPrintable(error));
	QVERIFY(!QFile::exists(created->path));
	QCOMPARE(environment.backup->backups(QStringLiteral("alpha")).size(), 0);

	const auto foreign = QDir(environment.dir.path()).filePath(QStringLiteral("foreign.db"));
	QFile file(foreign);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write("not a backup");
	file.close();
	QVERIFY(!environment.backup->removeBackup(QStringLiteral("alpha"), foreign, &error));
	QVERIFY(QFile::exists(foreign));
}

void TestDatabaseBackup::widgetAndDocumentShareSelectedStore() {
	TestEnvironment environment;
	environment.addStore(QStringLiteral("beta"));
	environment.addStore(QStringLiteral("alpha"));

	DatabaseBackupWidget widget(environment.backup.get());
	QCOMPARE(widget.selectedStore(), QStringLiteral("alpha"));
	QVERIFY(widget.selectStore(QStringLiteral("beta")));
	QCOMPARE(widget.selectedStore(), QStringLiteral("beta"));
	QVERIFY(!widget.selectStore(QStringLiteral("unknown")));

	DatabaseBackupDocument document(environment.backup.get());
	const auto context = environment.controller->globalContext()
		->createDocumentContext(QStringLiteral("database-backup"));
	context->setVariable(QStringLiteral("store"), QStringLiteral("beta"));
	document.setContext(context);
	QCOMPARE(document.widget()->selectedStore(), QStringLiteral("beta"));

	document.widget()->selectStore(QStringLiteral("alpha"));
	QCOMPARE(context->variable(QStringLiteral("store")).toString(), QStringLiteral("alpha"));
}

void TestDatabaseBackup::deleteButtonIsRightAlignedAndRed() {
	TestEnvironment environment;
	environment.addStore(QStringLiteral("alpha"));
	QVERIFY(environment.databases->connection(QStringLiteral("alpha")));
	QString error;
	QVERIFY2(environment.backup->createBackup(QStringLiteral("alpha"), &error).has_value(),
		qPrintable(error));

	// Проверяем именно оформление из темы: кнопка удаления красная и не растянута.
	QFile stylesheet(QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR)
		+ QStringLiteral("/assets/themes/dark/stylesheets/database-backup.qss"));
	QVERIFY(stylesheet.open(QIODevice::ReadOnly));
	qApp->setStyleSheet(QString::fromUtf8(stylesheet.readAll()));

	DatabaseBackupWidget widget(environment.backup.get());
	widget.resize(640, 520);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));

	auto* create = widget.findChild<QPushButton*>(QStringLiteral("DatabaseBackupCreate"));
	auto* remove = widget.findChild<QPushButton*>(QStringLiteral("DatabaseBackupDelete"));
	QVERIFY(create);
	QVERIFY(remove);
	QVERIFY(remove->isVisible());

	// Кнопка справа от «Создать копию», прижата к правому краю и не растянута.
	QVERIFY(remove->geometry().left() > create->geometry().right());
	QVERIFY(remove->width() <= create->width());
	const int contentRight = widget.contentsRect().right() - 8;
	QVERIFY2(qAbs(remove->geometry().right() - contentRight) <= 12,
		qPrintable(QStringLiteral("right=%1 content=%2")
			.arg(remove->geometry().right()).arg(contentRight)));

	// Красный фон: точка слева от текста (внутри скругления и padding).
	const auto image = remove->grab().toImage();
	const auto color = image.pixelColor(6, image.height() / 2);
	QVERIFY2(color.red() > 180 && color.green() < 90 && color.blue() < 90,
		qPrintable(color.name()));

	widget.close();
	qApp->setStyleSheet(QString());
}

QTEST_MAIN(TestDatabaseBackup)
#include "test_database_backup.moc"
