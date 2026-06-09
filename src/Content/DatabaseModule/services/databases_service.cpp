#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/DatabaseModule/data_providers/i_databases_settings_data_provider.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_wal_manager.h"

#include <QApplication>
#include <QDir>

#include <map>

class DatabasesService::Private {
public:
	Private(DatabasesService* parent) : q(parent) {}
	DatabasesService* q;

	struct DatabaseEntry {
		std::unique_ptr<SQLiteConnection> connection;
		std::unique_ptr<SQLiteWalManager> walManager;
		bool initialized = false;
	};

	std::map<QString, DatabaseEntry> entries;
	Resources* resources;
	IDatabaseSettingsDataProvider* settingsDataProvider;
};

DatabasesService::DatabasesService(Resources* resources, IDatabaseSettingsDataProvider* settingsDataProvider, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
	d->settingsDataProvider = settingsDataProvider;
	// shutdown application
	connect(qApp, &QApplication::aboutToQuit, this, &DatabasesService::onApplicationShutdown);
}

DatabasesService::~DatabasesService() = default;

void DatabasesService::onApplicationShutdown() {
	shutdown();
}

void DatabasesService::shutdown() {
	for (auto& entry : d->entries) {
		auto& db = entry.second;

		if (db.initialized) {
			// Останавливаем авто-checkpoint
			db.walManager->stopAutoCheckpoint();

			// Финальный checkpoint
			db.walManager->checkpoint(SQLiteWalManager::TRUNCATE);

			// Оптимизация
			db.connection->execute("PRAGMA optimize");

			// Закрываем соединение
			db.connection.reset();
		}
	}

	d->entries.clear();
	qInfo() << "All databases shut down";
}

SQLiteConnection* DatabasesService::connection(const QString& name) {
	const auto identName = name.toLower();
	const auto& db = d->entries.find(identName);
	if (db != d->entries.end()) {
		return db->second.connection.get();
	}

	const auto settings = d->settingsDataProvider->get(name);
	if (!settings) {
		qCritical() << "Failed to read database settings:" << name;
		return nullptr;
	}

	Private::DatabaseEntry entry;
	entry.connection = std::make_unique<SQLiteConnection>(d->resources);
	if (!entry.connection->open(identName)) {
		qCritical() << "Failed to open database:" << name;
		return nullptr;
	}

	entry.walManager = std::make_unique<SQLiteWalManager>(entry.connection.get());
	entry.walManager->startAutoCheckpoint(
		settings->walCheckInterval,
		settings->walMaxSizeMb);
	entry.initialized = true;

	const auto& inserted = d->entries.emplace(identName, std::move(entry));
	return inserted.first->second.connection.get();
}
