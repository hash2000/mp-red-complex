#pragma once
#include "Engine/main_frame.h"
#include "Resources/resources.h"
#include <memory>


class LauncherMainFrame : public MainFrame {
public:
	LauncherMainFrame(std::shared_ptr<Resources> resources);

private:
	std::shared_ptr<Resources> _resources;
};
