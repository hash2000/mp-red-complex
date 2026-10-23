#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/DatabaseModule/data_providers/i_databases_settings_data_provider.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_wal_manager.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/resources.h"
#include "Libs/Resources/directories/directories_context.h"
#include "Libs/Base/crypto/security_buffer.h"

#include "Content/DatabaseModule/migrations/game_migrations.h"
#include "Content/DatabaseModule/migrations/accounts_migrations.h"
#include "Content/DatabaseModule/migrations/fetch_api_migrations.h"

#include <QApplication>
#include <QDir>

#include <functional>
#include <map>

class DatabasesService::Private {
public:
	Private(DatabasesService* parent) : q(parent) {}
	DatabasesService* q;

	struct DatabaseEntry {
		std::unique_ptr<SQLiteConnection> connection;
		std::unique_ptr<SQLiteWalManager> walManager;
		std::unique_ptr<MigrationManager> migrator;
		bool initialized = false;
	};

	struct DatabaseAlias {
		QString path;
		bool encrypted = false;
	};

	std::unique_ptr<SecureBuffer> encryptionKey; // Ключ для SQLCipher (32 байта)
	std::map<QString, DatabaseAlias> aliases;
	std::map<QString, DatabaseEntry> entries;
	std::map<QString, std::function<void (MigrationManager*)>> migrations;
	Resources* resources;
	IDatabaseSettingsDataProvider* settingsDataProvider;

	void appendAlias(DirectoryPath path, const QString& alias, bool encripted);
};

DatabasesService::DatabasesService(Resources* resources,
	IDatabaseSettingsDataProvider* settingsDataProvider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
	d->settingsDataProvider = settingsDataProvider;

	d->migrations = {
		{ "game", [](MigrationManager* manager) { GameMigrations::build(manager); } },
		{ "accounts", [](MigrationManager* manager) { AccountsMigrations::build(manager); } },
		{ "fetch_api", [](MigrationManager* manager) { FetchApiMigrations::build(manager); } },
	};

	reloadAliases();

	// shutdown application
	connect(qApp, &QApplication::aboutToQuit, this, &DatabasesService::onApplicationShutdown);
}

DatabasesService::~DatabasesService() = default;


void DatabasesService::onApplicationShutdown() {
	shutdown();
}

void DatabasesService::reloadAliases() {
	d->aliases.clear();
	d->appendAlias(DirectoryPath::AccountsDbFile, "accounts", false);
	d->appendAlias(DirectoryPath::GameDbFile, "game", true);
	d->appendAlias(DirectoryPath::FetchApiDbFile, "fetch_api", false);
	d->appendAlias(DirectoryPath::MessangerDbFile, "messanger", true);
}

void DatabasesService::Private::appendAlias(DirectoryPath path, const QString& alias, bool encrypted) {
	 const auto dir = resources->Directories.get(path);
	 if (!dir) {
		 return;
	 }

	 auto &it = aliases[alias.toLower()];
	 it.path = dir.value().absolutePath();
	 it.encrypted = encrypted;
}

void DatabasesService::shutdown() {
	for (auto& entry : d->entries) {
		auto& db = entry.second;

		if (db.initialized) {			
			db.walManager->stopAutoCheckpoint(); // Останавливаем авто-checkpoint
			db.walManager->checkpoint(SQLiteWalManager::TRUNCATE); // Финальный checkpoint
			db.connection->execute("PRAGMA optimize");// Оптимизация
			db.connection.reset(); // Закрываем соединение
		}
	}

	d->entries.clear();
	qInfo() << "All databases shut down";
}

SQLiteConnection* DatabasesService::connection(const QString& name) {
	const auto identName = name.toLower();
	const auto dbPathIt = d->aliases.find(identName);

	if (dbPathIt == d->aliases.end()) {
		qCritical() << "Undefined database alias:" << name;
		return nullptr;
	}

	const auto dbAlies = dbPathIt->second;
	const auto dbPath = dbAlies.path;

	QFileInfo fileInfo(dbPath);
	if (!fileInfo.absoluteDir().exists()) {
		if (!QDir().mkpath(fileInfo.absolutePath())) {
			qCritical() << "Failed to create directory:" << fileInfo.absolutePath();
			return nullptr;
		}
	}

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
	entry.connection = std::make_unique<SQLiteConnection>();
	auto connectionString = QString("file=%1;").arg(dbPath);

	if (dbAlies.encrypted) {
		connectionString += QString("token=%1;")
			.arg(d->encryptionKey->toHex());
	}

	if (!entry.connection->open(connectionString)) {
		qCritical() << "Failed to open database:" << name;
		return nullptr;
	}

	entry.walManager = std::make_unique<SQLiteWalManager>(entry.connection.get());
	entry.walManager->startAutoCheckpoint(
		settings->walCheckInterval,
		settings->walMaxSizeMb);
	entry.migrator = std::make_unique<MigrationManager>();
	entry.initialized = true;

	const auto& inserted = d->entries.emplace(identName, std::move(entry));
	auto conn = inserted.first->second.connection.get();

	auto migrator = inserted.first->second.migrator.get();

	const auto& migratorInitializer = d->migrations.find(name);
	if (migratorInitializer != d->migrations.end()) {
		migratorInitializer->second(migrator);
	}

	if (migrator->needsMigration(*conn)) {
		if (!migrator->migrate(*conn)) {
			qCritical() << "Database migration error:" << name;
			return nullptr;
		}
	}	

	return conn;
}

void DatabasesService::setEncryptionKey(const QByteArray& data) {
	if (data.size() == 0) {
		return;
	}

	if (!d->encryptionKey) {
		d->encryptionKey = std::make_unique<SecureBuffer>(data.size());
	}

	d->encryptionKey->setData(data);
	return;
}

void DatabasesService::cleadEncryptionKey() {

	shutdown();
	d->encryptionKey->clear();
}
