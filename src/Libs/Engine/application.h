#pragma once
#include <QString>

#include <memory>

class MainFrame;
class Resources;
class Config;

class Application {
public:
	Application();
	virtual ~Application();

protected:
	virtual QString organizationName() const = 0;
	virtual QString organizationDomain() const = 0;
	virtual QString applicationName() const = 0;

public:
	int run(int &argc, char **argv);

private:
	class Private;
	std::unique_ptr<Private> d;
};
