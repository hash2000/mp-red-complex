#pragma once
#include <QString>
#include <QtGlobal>
#include <list>

namespace BootstrapConfig {

struct Application {
	QString name;
	QString viewport;
};

/// Блок appearance: оформление и описываемое приложение. В bootstrap.yaml
/// это значения по умолчанию, из которых создаётся пустой профиль
/// пользователя (profiles/<имя>/appearance.yaml).
struct Appearance {
	QString themeName;
	Application application;
};

struct Migration {
	qint64 version = 0;
	QString description;
	QString up;
	QString down;
};

struct Store {
	QString name;
	QString path;
	bool encrypted = false;
	std::list<Migration> migrations;

	struct {
		int checkInterval = 60000;
		int maxSizeMb = 20;
	} wal;
};

struct Config {
	Appearance appearance;
	std::list<Store> stores;
};

Config loadBootstrap(const QString& path);
}
