#pragma once
#include <memory>

class Resources;

class BootstrapManager {
public:
	BootstrapManager(Resources* resources);
	virtual ~BootstrapManager();

	bool reload();

private:
	class Private;
	std::unique_ptr<Private> d;
};
