#include "Content/DatabaseModule/services/databases_service.h"
#include "Libs/Base/crypto/security_buffer.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_wal_manager.h"
#include "Libs/Resources/resources.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QVariantMap>

#include <map>

namespace {
constexpr int kDefaultWalInterval = 60000;
constexpr int kDefaultWalMaxSizeMb = 20;

int positiveSetting(const std::shared_ptr<CommandContext>& context, const QString& name, int fallback) {
  if (!context) {
    return fallback;
  }
  bool ok = false;
  const int value = context->variable(name, fallback).toInt(&ok);
  if (!ok || value <= 0) {
    qWarning() << "Invalid database setting" << name << "; using default" << fallback;
    return fallback;
  }
  return value;
}

bool loadMigrations(MigrationManager& manager, const std::shared_ptr<CommandContext>& context) {
  if (!context || !context->hasVariable("migrations")) {
    return true;
  }
  const auto migrationsValue = context->variable("migrations");
  if (migrationsValue.metaType().id() != QMetaType::QVariantList) {
    qCritical() << "Invalid migrations list in" << context->nameSpace();
    return false;
  }
  QSet<qint64> versions;
  for (const auto& value : migrationsValue.toList()) {
    if (value.metaType().id() != QMetaType::QVariantMap) {
      qCritical() << "Invalid migration in" << context->nameSpace();
      return false;
    }
    const auto migration = value.toMap();
    bool ok = false;
    const qint64 version = migration.value("version").toLongLong(&ok);
    const auto up = migration.value("up").toString();
    const auto down = migration.value("down").toString();
    auto description = migration.value("description").toString();
    if (description.isEmpty()) {
      description = QString::number(version);
    }
    if (!ok || version <= 0 || versions.contains(version) || up.trimmed().isEmpty()) {
      qCritical() << "Invalid or duplicate database migration" << version << "in" << context->nameSpace();
      return false;
    }
    versions.insert(version);
    manager.addMigration(version, description,
      [up](SQLiteConnection& db) { return db.execute(up); },
      [down](SQLiteConnection& db) { return !down.trimmed().isEmpty() && db.execute(down); });
  }
  return true;
}
}

class DatabasesService::Private {
public:
  struct DatabaseEntry {
    std::unique_ptr<SQLiteConnection> connection;
    std::unique_ptr<SQLiteWalManager> walManager;
    std::unique_ptr<MigrationManager> migrator;
  };

  Resources* resources = nullptr;
  std::shared_ptr<CommandContext> globalContext;
  std::unique_ptr<SecureBuffer> encryptionKey;
  std::map<QString, DatabaseEntry> entries;
};

DatabasesService::DatabasesService(Resources* resources, std::shared_ptr<CommandContext> globalContext,
  QObject* parent)
  : QObject(parent), d(std::make_unique<Private>()) {
  d->resources = resources;
  d->globalContext = std::move(globalContext);
  if (QCoreApplication::instance()) {
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
      this, &DatabasesService::onApplicationShutdown);
  }
}

DatabasesService::~DatabasesService() {
  shutdown();
}

void DatabasesService::onApplicationShutdown() {
  shutdown();
}

void DatabasesService::reloadAliases() {
  shutdown();
}

void DatabasesService::shutdown() {
  for (auto& [name, entry] : d->entries) {
    entry.walManager->stopAutoCheckpoint();
    entry.walManager->checkpoint(SQLiteWalManager::TRUNCATE);
    entry.connection->execute("PRAGMA optimize");
  }
  // WAL-менеджер должен уничтожаться раньше соединения.
  d->entries.clear();
}

SQLiteConnection* DatabasesService::connection(const QString& name) {
  const auto alias = name.toLower();
  static const QRegularExpression validAlias("^[a-z0-9_-]+$");
  if (!validAlias.match(alias).hasMatch() || !d->resources) {
    qCritical() << "Invalid database alias or resources:" << name;
    return nullptr;
  }
  const auto existing = d->entries.find(alias);
  if (existing != d->entries.end()) {
    return existing->second.connection.get();
  }

  const auto context = d->globalContext ? d->globalContext->context(".store." + alias) : nullptr;
  const auto databasesDir = d->resources->Directories.get("path.databases");
  const auto profileDir = d->resources->Directories.get("path.profile");
  QString dbPath = context ? context->variable("path").toString() : QString();
  if (dbPath.isEmpty()) {
    if (!databasesDir) {
      qCritical() << "Database directory is not configured:" << name;
      return nullptr;
    }
    dbPath = databasesDir->filePath(alias + ".db");
  } else if (QDir::isRelativePath(dbPath)) {
    if (!profileDir) {
      qCritical() << "Profile directory is not configured:" << name;
      return nullptr;
    }
    dbPath = profileDir->filePath(dbPath);
  }
  dbPath = QFileInfo(dbPath).absoluteFilePath();
  if (dbPath.contains(';') || dbPath.contains('=')) {
    qCritical() << "Unsupported character in database path:" << dbPath;
    return nullptr;
  }
  if (!QDir().mkpath(QFileInfo(dbPath).absolutePath())) {
    qCritical() << "Failed to create database directory:" << dbPath;
    return nullptr;
  }

  const bool encrypted = context && context->variable("encrypted", false).toBool();
  if (encrypted && (!d->encryptionKey || d->encryptionKey->isEmpty())) {
    qCritical() << "Encryption key is required for database:" << name;
    return nullptr;
  }

  Private::DatabaseEntry entry;
  entry.migrator = std::make_unique<MigrationManager>();
  if (!loadMigrations(*entry.migrator, context)) {
    return nullptr;
  }
  entry.connection = std::make_unique<SQLiteConnection>();
  QString connectionString = QString("file=%1;").arg(dbPath);
  if (encrypted) {
    connectionString += QString("token=%1;").arg(QString::fromLatin1(d->encryptionKey->toHex()));
  }
  if (!entry.connection->open(connectionString)) {
    qCritical() << "Failed to open database:" << name;
    return nullptr;
  }
  if (!entry.migrator->migrate(*entry.connection)) {
    qCritical() << "Database migration error:" << name;
    return nullptr;
  }

  entry.walManager = std::make_unique<SQLiteWalManager>(entry.connection.get());
  entry.walManager->startAutoCheckpoint(
    positiveSetting(context, "wal.checkInterval", kDefaultWalInterval),
    positiveSetting(context, "wal.maxSizeMb", kDefaultWalMaxSizeMb));
  auto inserted = d->entries.emplace(alias, std::move(entry));
  return inserted.first->second.connection.get();
}

void DatabasesService::setEncryptionKey(const QByteArray& data) {
  if (data.isEmpty()) {
    return;
  }
  // Открытые соединения не должны продолжать работать со старым ключом.
  shutdown();
  if (!d->encryptionKey || d->encryptionKey->size() != static_cast<size_t>(data.size())) {
    d->encryptionKey = std::make_unique<SecureBuffer>(data.size());
  }
  d->encryptionKey->setData(data);
}

void DatabasesService::cleadEncryptionKey() {
  shutdown();
  if (d->encryptionKey) {
    d->encryptionKey->clear();
  }
}
