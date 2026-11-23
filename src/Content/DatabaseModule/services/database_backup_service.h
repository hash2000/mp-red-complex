#pragma once
#include "Content/DatabaseModule/models/database_backup.h"

#include <QList>
#include <QObject>
#include <QString>

#include <memory>
#include <optional>

class CommandContext;
class DatabasesService;
class Resources;

/// Резервное копирование и восстановление баз данных, зарегистрированных
/// в контекстах `.store.<alias>`.
///
/// Копия — файл базы в каталоге профиля (`backups/<alias>`). Файл копируется
/// как есть, поэтому шифрование SQLCipher сохраняется: копия не содержит
/// открытого текста, а для проверки и повторного открытия нужен тот же ключ,
/// что и у рабочей базы.
///
/// Перед копированием выполняется `PRAGMA wal_checkpoint(TRUNCATE)`, чтобы
/// снимок был цельным (WAL свёрнут в основной файл). Перед восстановлением
/// копия проверяется (`DatabasesService::verifyDatabaseFile`), соединение с
/// базой закрывается, а текущий файл сохраняется страховочной копией
/// (`*-before-restore.db`). Если восстановленная база не открывается,
/// изменения откатываются.
///
/// Все операции выполняются в потоке вызывающей стороны (GUI-поток): сервис
/// не создаёт фоновых потоков и не блокирует другие базы.
class DatabaseBackupService : public QObject {
	Q_OBJECT
public:
	DatabaseBackupService(Resources* resources, std::shared_ptr<CommandContext> globalContext,
		DatabasesService* databases, QObject* parent = nullptr);
	~DatabaseBackupService() override;

	/// Зарегистрированные хранилища (`.store.*`), отсортированы по имени.
	QList<DatabaseStoreInfo> stores() const;
	/// Описание одного хранилища; nullopt — имя не зарегистрировано.
	std::optional<DatabaseStoreInfo> store(const QString& alias) const;

	/// Копии хранилища, новые сверху.
	QList<DatabaseBackupInfo> backups(const QString& alias) const;
	/// Каталог копий хранилища (`<профиль>/backups/<alias>`).
	QString backupDirectory(const QString& alias) const;

	/// Создать копию базы. nullopt — операция не выполнена (см. error).
	std::optional<DatabaseBackupInfo> createBackup(const QString& alias, QString* error = nullptr);
	/// Заменить базу копией. false — операция не выполнена (см. error).
	bool restoreBackup(const QString& alias, const QString& backupPath, QString* error = nullptr);
	/// Удалить файл копии (только из каталога копий этого хранилища).
	bool removeBackup(const QString& alias, const QString& backupPath, QString* error = nullptr);

signals:
	void backupCreated(const QString& alias, const QString& path);
	void backupRestored(const QString& alias, const QString& path);
	/// Изменился список копий или их количество.
	void backupsChanged(const QString& alias);

private:
	class Private;
	std::unique_ptr<Private> d;
};
