#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QDir>
#include <QDebug>

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

void loadStores(const YAML::Node& root, Config& cfg) {
	const auto stores = root["stores"];
	if (!stores || !stores["items"]) {
		return;
	}

	const auto& items = stores["items"];
	if (!items.IsMap()) {
		throw std::runtime_error("Bootstrap parameter stores.items must be a map");
	}

	for (const auto& item : items) {
		Store store;
		store.name = QString::fromStdString(item.first.as<std::string>());

		const auto& body = item.second;
		const auto& wal = body["wal"];
		if (!wal) {
			cfg.stores.push_back(std::move(store));
			continue;
		}

		if (!wal.IsMap()) {
			throw std::runtime_error("Bootstrap parameter stores.items."
				+ store.name.toStdString() + ".wal must be a map");
		}

		if (wal["checkInterval"]) {
			store.wal.checkInterval = wal["checkInterval"].as<int>();
		}

		if (wal["maxSizeMb"]) {
			store.wal.maxSizeMb = wal["maxSizeMb"].as<int>();
		}

		qDebug() << "Bootstrap store:" << store.name
			<< "wal checkInterval:" << store.wal.checkInterval
			<< "maxSizeMb:" << store.wal.maxSizeMb;

		cfg.stores.push_back(std::move(store));
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
	loadStores(root, cfg);

	return cfg;
}
}
