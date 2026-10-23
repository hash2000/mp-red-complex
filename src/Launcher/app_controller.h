#pragma once
#include "Content/ConsoleModule/command_controller.h"
#include <memory>

class Resources;
class Controllers;

class ApplicationController : public CommandController {
Q_OBJECT
public:
	ApplicationController(Resources* resources, QObject* parent = nullptr);
	~ApplicationController();

	ServicesRegistry* services() const override;
	CommandProcessor* commandProcessor() const override;
	Controllers* controllers() const;

private:
	void initContext() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
