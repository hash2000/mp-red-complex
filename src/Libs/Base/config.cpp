#include "Libs/Base/config.h"
#include "Libs/Base/config/config_exception.h"
#include <qsettings.h>
#include <qstandardpaths.h>
#include <QApplication>

const Config _default_scheme = Config();

Config::Config()
	: color_scheme(Qt::ColorScheme::Dark)
	, application_name("Red Complex")
	, application_path_name("red-complex")
	, organization_name("Hash2000")
{
	cwd = QDir::current();
}

Config::Config(const Config &clone) : Config() {
	color_scheme = clone.color_scheme;
	cwd = clone.cwd;
	resources_path = clone.resources_path;
}

const Config &Config::getDefult() {
	return _default_scheme;
}

void Config::setupPath()
{
	const auto configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
	config_path = QDir(configPath);
	qDebug() << "Config path is:" << configPath;

	if (!QDir().mkpath(configPath)) {
		throw ConfigException(QString("Con't create config directiry [%1]")
			.arg(configPath));
	}
}

void Config::loadSettings() {
	const auto configFilePath = config_path.filePath("config.ini");
	const QFileInfo configFile(configFilePath);

	if (!configFile.exists()) {
		installDefaultConfigFile(configFilePath);
	}

	QSettings settings(configFilePath, QSettings::Format::IniFormat);
	resources_path = settings.value("resources/path").toString();

}

void Config::installDefaultConfigFile(const QString& path) {
	QFile file(path);
	if (!file.open(QFile::WriteOnly)) {
		throw ConfigException(QString("Can't open ") + path);
	}

	file.write("[logginng]\r\n");
	file.write("\r\n");
}
