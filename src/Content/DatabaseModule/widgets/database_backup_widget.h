#pragma once
#include <QWidget>

#include <memory>

class DatabaseBackupService;

/// Панель резервного копирования баз данных.
///
/// Самостоятельный виджет: не знает о документе и его контексте. Слева —
/// список хранилищ из `.store`, справа — файл базы и её копии. Все операции
/// выполняет `DatabaseBackupService`; виджет только показывает результат и
/// сообщает о выбранном хранилище сигналом `storeSelected`.
class DatabaseBackupWidget final : public QWidget {
	Q_OBJECT
public:
	explicit DatabaseBackupWidget(DatabaseBackupService* service, QWidget* parent = nullptr);
	~DatabaseBackupWidget() override;

	DatabaseBackupService* service() const;

	/// Выбранное хранилище (алиас `.store.<alias>`); пустая строка — не выбрано.
	QString selectedStore() const;
	/// Выбрать хранилище в списке. false — хранилище не зарегистрировано.
	bool selectStore(const QString& alias);

	/// Перечитать список хранилищ и копий, сохранив текущий выбор.
	void refresh();

public slots:
	void createBackup();
	void restoreSelectedBackup();
	void deleteSelectedBackup();
	void openBackupFolder();

signals:
	/// Пользователь выбрал хранилище (в том числе после обновления списка).
	void storeSelected(const QString& alias);
	/// Создана копия.
	void backupCreated(const QString& alias, const QString& path);
	/// База восстановлена из копии.
	void databaseRestored(const QString& alias, const QString& path);
	/// Операция завершилась ошибкой; сообщение уже показано в статусе.
	void errorOccurred(const QString& message);
	void statusChanged(const QString& message, bool error);

private:
	class Private;
	std::unique_ptr<Private> d;
};
