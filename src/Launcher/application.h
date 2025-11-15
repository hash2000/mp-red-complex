#pragma once
#include "Base/config.h"
#include "Launcher/resources.h"
#include <QApplication>
#include <memory>

class Application {
public:
	virtual ~Application() = default;
	static std::unique_ptr<Application> create();

public:
	int run(int &argc, char **argv);

private:
	int tryRun(int &argc, char **argv);

	void installMessageHandler();

private:
	std::shared_ptr<Config> _config;
	std::shared_ptr<Resources> _resources;
};
