#pragma once
#include "Base/config.h"
#include "Engine/main_frame.h"
#include "Resources/resources.h"
#include <QApplication>
#include <memory>

class Application {
public:
	virtual ~Application() = default;

protected:
	virtual std::unique_ptr<MainFrame> createMainFrame() = 0;

	std::shared_ptr<Resources> resources();

public:
	int run(int &argc, char **argv);

private:
	int tryRun(int &argc, char **argv);

	void installMessageHandler();

private:
	std::shared_ptr<Config> _config;
	std::shared_ptr<Resources> _resources;
};
