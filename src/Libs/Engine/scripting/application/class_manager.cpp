#include "Libs/Engine/scripting/application/class_manager.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Resources/resources.h"

#include <yaml-cpp/yaml.h>

#include <QFile>

class ClassManager::Private {
public:

};


ClassManager::ClassManager(Resources* resources, BootstrapConfig::Application& application)
	: d(std::make_unique<Private>()) {
	try {
		auto basePath = resources->Directories.get("path.profile");
		if (!basePath) {
			throw std::runtime_error("Bootstrap path is not set");
		}

		auto path = basePath->filePath("applications/" + application.name + "/main.yaml");
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			throw std::runtime_error("Cannot open application config: " + path.toStdString());
		}

		YAML::Node root = YAML::Load(file.readAll().toStdString());


	}
	catch (const std::exception& e) {
		qCritical() << "Fatal error during locad application" << application.name << ":" << e.what();
	}
}

ClassManager::~ClassManager() = default;
