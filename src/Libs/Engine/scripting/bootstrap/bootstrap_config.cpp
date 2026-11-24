#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

#include <utility>

namespace BootstrapConfig {

void loadTheme(const YAML::Node& root, Config& cfg) {
	if (root["theme"]) {
		cfg.themeName = QString::fromStdString(root["theme"].as<std::string>());
	}
	else {
		cfg.themeName = "dark";
	}
}

void loadApplication(const YAML::Node& root, Config& cfg) {
	if (!root["application"]) {
		throw std::runtime_error("Need Bootstrap parameter: appearance.application");
	}

	const auto& app = root["application"];
	if (!app["name"]) {
		throw std::runtime_error("Need Bootstrap parameter: appearance.application.name");
	}

	if (!app["viewport"] || !app["viewport"]["xtype"]) {
		throw std::runtime_error("Need Bootstrap parameter: appearance.application.viewport.xtype");
	}

	cfg.application.name = QString::fromStdString(app["name"].as<std::string>());
	cfg.application.viewport = QString::fromStdString(app["viewport"]["xtype"].as<std::string>());
}

QString readMigrationFile(const QDir& configDir, const QString& path) {
	QFile file(configDir.filePath(path));
	if (path.isEmpty() || !file.open(QIODevice::ReadOnly)) {
		throw std::runtime_error("Cannot open migration file: " + file.fileName().toStdString());
	}
	const auto contents = file.readAll();
	if (file.error() != QFileDevice::NoError) {
		throw std::runtime_error("Cannot read migration file: " + file.fileName().toStdString());
	}
	return QString::fromUtf8(contents);
}

void loadMigrations(const YAML::Node& node, const QDir& configDir, Store& store) {
	if (!node) {
		return;
	}
	if (!node.IsSequence()) {
		throw std::runtime_error("Bootstrap migrations must be a sequence");
	}

	QSet<qint64> versions;
	for (const auto& entry : node) {
		if (!entry.IsMap() || !entry["version"] || !entry["up"] || !entry["down"]) {
			throw std::runtime_error("Migration requires version, up and down");
		}
		const auto versionText = QString::fromStdString(entry["version"].as<std::string>());
		static const QRegularExpression versionPattern("^[0-9]{14}$");
		if (!versionPattern.match(versionText).hasMatch() ||
			!QDate::fromString(versionText.left(8), "yyyyMMdd").isValid() ||
			!QTime::fromString(versionText.mid(8), "HHmmss").isValid()) {
			throw std::runtime_error("Migration version must be a valid YYYYMMDDhhmmss timestamp");
		}
		Migration migration;
		migration.version = versionText.toLongLong();
		if (versions.contains(migration.version)) {
			throw std::runtime_error("Duplicate migration version: " + versionText.toStdString());
		}
		versions.insert(migration.version);
		migration.description = entry["description"]
			? QString::fromStdString(entry["description"].as<std::string>()) : QString();
		const auto upPath = QString::fromStdString(entry["up"].as<std::string>());
		const auto downPath = QString::fromStdString(entry["down"].as<std::string>());
		const QRegularExpression upPattern("^" + versionText + "-up(?:-.+)?\\.sql$");
		if (!upPattern.match(QFileInfo(upPath).fileName()).hasMatch() ||
			QFileInfo(downPath).fileName() != versionText + "-down.sql") {
			throw std::runtime_error("Migration filenames must match the version and direction: " + versionText.toStdString());
		}
		migration.up = readMigrationFile(configDir, upPath);
		migration.down = readMigrationFile(configDir, downPath);
		store.migrations.push_back(std::move(migration));
	}
	store.migrations.sort([](const Migration& a, const Migration& b) { return a.version < b.version; });
}

// Настройки одного хранилища лежат в отдельном файле stores/<name>.yaml
// рядом с bootstrap.yaml. Пути SQL-миграций по-прежнему разрешаются
// относительно каталога bootstrap.yaml.
Store loadStore(const QString& name, const QDir& configDir) {
	const auto storePath = configDir.filePath(QStringLiteral("stores/%1.yaml").arg(name));
	QFile file(storePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw std::runtime_error("Cannot open store config: " + storePath.toStdString());
	}
	const auto contents = file.readAll();
	if (file.error() != QFileDevice::NoError) {
		throw std::runtime_error("Cannot read store config: " + storePath.toStdString());
	}

	YAML::Node body;
	try {
		body = YAML::Load(contents.toStdString());
	}
	catch (const YAML::Exception& error) {
		throw std::runtime_error("Invalid YAML in store config " + storePath.toStdString()
			+ ": " + error.what());
	}

	Store store;
	store.name = name;
	// Пустой файл (или только комментарии) — хранилище с настройками
	// по умолчанию.
	if (body.IsNull()) {
		return store;
	}
	if (!body.IsMap()) {
		throw std::runtime_error("Store config must be a map: " + storePath.toStdString());
	}

	if (body["name"]) {
		const auto declared = QString::fromStdString(body["name"].as<std::string>()).toLower();
		if (declared != name) {
			throw std::runtime_error("Store config name does not match its file name: "
				+ storePath.toStdString());
		}
	}
	if (body["path"]) {
		store.path = QString::fromStdString(body["path"].as<std::string>());
	}
	if (body["encrypted"]) {
		store.encrypted = body["encrypted"].as<bool>();
	}
	loadMigrations(body["migrations"], configDir, store);

	const auto& wal = body["wal"];
	if (!wal) {
		return store;
	}
	if (!wal.IsMap()) {
		throw std::runtime_error("Store parameter wal must be a map: "
			+ storePath.toStdString());
	}
	if (wal["checkInterval"]) {
		store.wal.checkInterval = wal["checkInterval"].as<int>();
	}
	if (wal["maxSizeMb"]) {
		store.wal.maxSizeMb = wal["maxSizeMb"].as<int>();
	}

	return store;
}

void loadStores(const YAML::Node& root, Config& cfg, const QDir& configDir) {
	const auto stores = root["stores"];
	if (!stores || !stores["items"]) {
		return;
	}

	// В bootstrap.yaml остаётся только перечисление имён: настройки хранилищ
	// вынесены в stores/<name>.yaml. Чтобы отключить хранилище, достаточно
	// убрать его имя из списка — файл настроек при этом сохраняется.
	const auto& items = stores["items"];
	if (!items.IsSequence()) {
		throw std::runtime_error("Bootstrap parameter stores.items must be a sequence "
			"of store names; store settings live in stores/<name>.yaml");
	}

	QSet<QString> names;
	static const QRegularExpression validName("^[a-z0-9_-]+$");
	for (const auto& item : items) {
		if (!item.IsScalar()) {
			throw std::runtime_error("Bootstrap store name must be a string");
		}
		const auto name = QString::fromStdString(item.as<std::string>()).toLower();
		if (!validName.match(name).hasMatch() || names.contains(name)) {
			throw std::runtime_error("Invalid or duplicate store name: " + name.toStdString());
		}
		names.insert(name);
		cfg.stores.push_back(loadStore(name, configDir));
	}
}

Config loadBootstrap(const QString& path) {
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw std::runtime_error("Cannot open bootstrap config: " + path.toStdString());
	}

	const YAML::Node root = YAML::Load(file.readAll().toStdString());

	if (!root["appearance"]) {
		throw std::runtime_error("Need Bootstrap parameter: appearance");
	}

	Config cfg;
	const YAML::Node appearance = root["appearance"];

	loadTheme(appearance, cfg);
	loadApplication(appearance, cfg);
	loadStores(root, cfg, QFileInfo(path).absoluteDir());

	return cfg;
}
}
