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
		if (root["appearance"]["processor"]) {
			cfg.processorPath = QString::fromStdString(root["appearance"]["processor"].as<std::string>());
		}
	}
	return cfg;
}
}
