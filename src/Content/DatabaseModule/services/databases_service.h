#pragma once
#include <QObject>
#include <QString>
#include <memory>

class CommandContext;
class SQLiteConnection;
class Resources;

class DatabasesService : public QObject {
  Q_OBJECT
public:
  DatabasesService(Resources* resources, std::shared_ptr<CommandContext> globalContext,
    QObject* parent = nullptr);
  ~DatabasesService() override;

  void setEncryptionKey(const QByteArray& data);
  void cleadEncryptionKey();

  SQLiteConnection* connection(const QString& name);
  // Закрывает открытые соединения; следующие вызовы читают актуальные контексты.
  void reloadAliases();

  // Путь к файлу базы по её имени — без открытия соединения. Настройки
  // (`path` в контексте `.store.<alias>`, иначе `data/<alias>.db` профиля)
  // читаются так же, как в connection(). Пустая строка — имя некорректно
  // или не настроены каталоги ресурсов.
  QString databasePath(const QString& name) const;

  // Закрыть соединение с одной базой: WAL сворачивается в основной файл.
  // false — соединения с этой базой не было.
  bool closeDatabase(const QString& name);

  // Проверить файл базы: открывается ли он (с учётом шифрования хранилища)
  // и проходит ли PRAGMA integrity_check. Используется перед восстановлением,
  // чтобы не заменить рабочую базу повреждённым файлом.
  bool verifyDatabaseFile(const QString& name, const QString& path, QString* error = nullptr) const;

private:
  void shutdown();

private slots:
  void onApplicationShutdown();

private:
  class Private;
  std::unique_ptr<Private> d;
};
