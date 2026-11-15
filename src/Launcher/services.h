#pragma once
#include <memory>

class ServicesRegistry;
class Resources;
class CommandContext;

class Services {
public:
	Services(Resources* resources, std::shared_ptr<CommandContext> globalContext);
	~Services();

	std::unique_ptr<ServicesRegistry> create();

private:
	class Private;
	std::unique_ptr<Private> d;
};

