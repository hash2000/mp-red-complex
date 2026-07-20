#pragma once
#include <QObject>
#include <memory>

class SQLiteConnection;
class Resources;
class IDatabaseSettingsDataProvider;

class DatabasesService : public QObject {
	Q_OBJECT
public:
	DatabasesService(Resources* resources,
		IDatabaseSettingsDataProvider* settingsDataProvider,
		QObject* parent = nullptr);
	~DatabasesService();

	void setEncryptionKey(const QByteArray& data);
	void cleadEncryptionKey();

	SQLiteConnection* connection(const QString& name);
	void reloadAliases();

private:
	void shutdown();

private slots:
	void onApplicationShutdown();

private:
	class Private;
	std::unique_ptr<Private> d;
};
