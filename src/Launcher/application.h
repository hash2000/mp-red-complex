#pragma once
#include <QApplication>
#include <memory>
#include "config.h"

class Application {
public:
	virtual ~Application() = default;
	static std::unique_ptr<Application> create();

public:
	int run(int &argc, char **argv);

private:
	void init();

private:
	std::unique_ptr<Config> _config;
};
