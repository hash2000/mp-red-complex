#pragma once
#include <memory>

class Resources;
namespace BootstrapConfig {
struct Application;
}

class ClassManager {
public:
	ClassManager(Resources* resources, BootstrapConfig::Application& application);
	virtual ~ClassManager();


private:
	class Private;
	std::unique_ptr<Private> d;
};
