#pragma once
#include <QObject>
#include <memory>

class MigrationManager;
class SQLiteConnection;
class Resources;
class IDatabaseSettingsDataProvider;

class DatabasesService : public QObject {
	Q_OBJECT
public:
	DatabasesService(IDatabaseSettingsDataProvider* settingsDataProvider, QObject* parent = nullptr);
	~DatabasesService();

	SQLiteConnection* connection(const QString& name);

private:
	void shutdown();

private slots:
	void onApplicationShutdown();

private:
	class Private;
	std::unique_ptr<Private> d;
};
