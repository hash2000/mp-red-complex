#pragma once
#include <QString>
#include <memory>

class ApplicationController;
class MdiChildWindow;

class WindowsBuilder {
public:
	WindowsBuilder(ApplicationController* appController);
	~WindowsBuilder();

	MdiChildWindow* build(const QString& name);

private:
	class Private;
	std::unique_ptr<Private> d;
};
