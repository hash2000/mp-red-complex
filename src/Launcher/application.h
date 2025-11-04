#pragma once
#include "Launcher/config.h"
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
	std::unique_ptr<Config> _config;
	std::unique_ptr<Resources> _resources;
};
