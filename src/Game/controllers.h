#pragma once
#include <memory>

class WindowsController;

class Controllers {
public:
	Controllers();
	~Controllers();

	WindowsController* windowsController() const;


private:
	class Private;
	std::unique_ptr<Private> d;
};
