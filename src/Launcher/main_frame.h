#pragma once
#include "Engine/main_frame.h"
#include "Resources/resources.h"
#include <QStackedWidget>
#include <memory>


class LauncherMainFrame : public MainFrame {
public:
	LauncherMainFrame(std::shared_ptr<Resources> resources);

	virtual ~LauncherMainFrame() = default;

private:
	std::shared_ptr<Resources> _resources;
};
