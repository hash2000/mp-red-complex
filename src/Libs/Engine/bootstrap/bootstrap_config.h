#pragma once
#include <QString>

namespace BootstrapConfig {
struct Config {
	QString themeName;
	QString processorPath;
};

Config loadBootstrap(const QString& path);
}

