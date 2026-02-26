#pragma once
#include <memory>

class MainFrame;
class Resources;

class Application {
public:
	Application();
	virtual ~Application();

protected:
	virtual std::unique_ptr<MainFrame> createMainFrame() = 0;

	Resources* resources();

public:
	int run(int &argc, char **argv);

private:
	int tryRun(int &argc, char **argv);

	void installMessageHandler();

private:
	class Private;
	std::unique_ptr<Private> d;
};
