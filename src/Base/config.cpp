#include "Base/config.h"
#include "Base/config/config_exception.h"
#include <qsettings.h>
#include <qstandardpaths.h>
#include <QApplication>

const Config _default_scheme = Config();

Config::Config()
	: color_scheme(Qt::ColorScheme::Dark)
	, application_name("Red Complex")
	, application_path_name("red-complex")
	, organization_name("Hash2000")
	, app_session("Undefined")
{
	cwd = QDir::current();
}

Config::Config(const Config &clone) : Config() {
	color_scheme = clone.color_scheme;
	app_session = clone.app_session;
	cwd = clone.cwd;
	resources_path = clone.resources_path;
	resources_path_raw = clone.resources_path_raw;
}

const Config &Config::getDefult() {
	return _default_scheme;
}

void Config::setupPath()
{
	const auto configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
	config_path = QDir(configPath);

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
	app_session = settings.value("session/view").toString();
	resources_path = settings.value("resources/path").toString();
	resources_path_raw = settings.value("resources/path-raw").toString();
	resources_encoding = settings.value("system/encoding", "Windows-1251").toString();

	if (resources_path_raw.isEmpty()) {
		resources_path_raw = resources_path;
	}
}

void Config::installDefaultConfigFile(const QString& path) {
	QFile file(path);
	if (!file.open(QFile::WriteOnly)) {
		throw ConfigException(QString("Can't open ") + path);
	}

	file.write("[logginng]\r\n");
	file.write("\r\n");
	file.write("[session]\r\n");
	file.write("view=ResourcesView\r\n");
}
