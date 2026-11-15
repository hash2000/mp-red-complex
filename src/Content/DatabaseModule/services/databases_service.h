#pragma once
#include <QObject>
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

private:
  void shutdown();

private slots:
  void onApplicationShutdown();

private:
  class Private;
  std::unique_ptr<Private> d;
};
