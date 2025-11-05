#include "Launcher/config.h"
#include "Launcher/config/config_exception.h"
#include <qsettings.h>
#include <qstandardpaths.h>
#include <QApplication>

const Config _default_scheme = Config();

Config::Config()
	: color_scheme(Qt::ColorScheme::Dark)
	, application_name("Red Complex")
	, application_path_name("red-complex")
	, organization_name("Hash2000")
	, app_session(AppSession::Test)
{
	cwd = QDir::current();
}

Config::Config(const Config &clone) : Config() {
	color_scheme = clone.color_scheme;
	app_session = clone.app_session;
	cwd = clone.cwd;
}

const Config &Config::getDefult() {
	return _default_scheme;
}

void Config::setupPath()
{
	const auto configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
	config_path = QDir(configPath);

	if (!QDir().mkpath(configPath)) {
		QString msg = "Con't create config directiry [" + configPath + "]";
		throw ConfigException(msg);
	}
}

void Config::loadSettings() {
	const auto configFilePath = config_path.filePath("config.ini");
	const QFileInfo configFile(configFilePath);

	if (!configFile.exists()) {
		installDefaultConfigFile(configFilePath);
	}

	QSettings settings(configFilePath, QSettings::Format::IniFormat);
	app_session = From<AppSession>::from(settings.value("session/view").toString())
		.value_or(AppSession::Test);
	resources_path = settings.value("resources/path").toString();
	resources_language = settings.value("resources/language", "english").toString();
	resources_entry_point = settings.value("resources/entry-point", "master.dat").toString();
}

void Config::installDefaultConfigFile(const QString& path) {
	QFile file(path);
	if (!file.open(QFile::WriteOnly)) {
		throw ConfigException(QString("Can't open ") + path);
	}

	file.write("[logginng]\r\n");
	file.write("\r\n");
	file.write("[session]\r\n");
	file.write("view=ResourceView\r\n");
}
