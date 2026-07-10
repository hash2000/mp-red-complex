#pragma once
#include "Content/ConsoleModule/command_controller.h"
#include <memory>

class Resources;
class Controllers;
class CommandProcessor;

class ApplicationController : public CommandController {
Q_OBJECT
public:
	ApplicationController(Resources* resources, CommandProcessor* commandProcessor, QObject* parent = nullptr);
	~ApplicationController();

	std::unique_ptr<ServicesRegistry> createServices() override;
	Controllers* controllers() const;

private:
	void initContext() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
