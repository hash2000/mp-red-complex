#pragma once
#include <QStyleHints>
#include <QString>
#include <QDir>
#include <memory>

class Config {
public:
	Config();

	Config(const Config& clone);

	virtual ~Config() = default;

	static const Config& getDefult();

	void setupPath();

	void loadSettings();

private:
	void installDefaultConfigFile(const QString& path);

public:
	Qt::ColorScheme color_scheme;
	const QString application_name;
	const QString application_path_name;
	const QString organization_name;
	QDir cwd;
	QDir config_path;
	QString app_session;
	QString resources_path;
	QString resources_path_raw;
	QString resources_language;
};

class Configurable {
public:
	virtual ~Configurable() = default;

	virtual void configure(const std::shared_ptr<Config> &config) = 0;
};

