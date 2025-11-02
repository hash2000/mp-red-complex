#pragma once
#include "Launcher/application/application_session.h"
#include <QStyleHints>
#include <qdir.h>

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
	AppSession app_session;
};

class Configured {
public:
	virtual ~Configured() = default;

	virtual void configure(const Config& config) = 0;
};

