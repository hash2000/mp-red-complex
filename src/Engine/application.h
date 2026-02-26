#pragma once
#include <memory>

class MainFrame;
class Resources;
class Config;

class Application {
public:
	Application();
	virtual ~Application();

protected:
	virtual std::unique_ptr<MainFrame> createMainFrame(Resources* resources) = 0;

public:
	int run(int &argc, char **argv);

private:
	class Private;
	std::unique_ptr<Private> d;
};
