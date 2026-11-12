#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QDir>
#include <QDebug>

namespace BootstrapConfig {
Config loadBootstrap(const QString& path) {
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw std::runtime_error("Cannot open bootstrap config: " + path.toStdString());
	}

	YAML::Node root = YAML::Load(file.readAll().toStdString());

	Config cfg;
	if (root["appearance"]) {
		if (root["appearance"]["theme"]) {
			cfg.themeName = QString::fromStdString(root["appearance"]["theme"].as<std::string>());
		}
		else {
			cfg.themeName = "dark";
		}

		if (root["appearance"]["application"]) {
			const auto& app = root["appearance"]["application"];
			if (!app["name"]) {
				throw std::runtime_error("Need Bootstrap parameter: appearance.application.name");
			}

			if (!app["viewport"] || !app["viewport"]["xtype"]) {
				throw std::runtime_error("Need Bootstrap parameter: appearance.application.viewport.xtype");
			}

			cfg.application.name = QString::fromStdString(app["name"].as<std::string>());
			cfg.application.viewport = QString::fromStdString(app["viewport"]["xtype"].as<std::string>());
		}
	}

	return cfg;
}
}
