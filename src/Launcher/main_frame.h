#pragma once
#include "Launcher/widgets/legend_widget.h"
#include "Launcher/widgets/actions_widget.h"
#include "Launcher/widgets/progress_widget.h"
#include "Launcher/map/map_view.h"
#include "Launcher/tasks/task_manager.h"
#include "Engine/main_frame.h"
#include "Resources/resources.h"
#include <QStackedWidget>
#include <QTimer>
#include <memory>

class LauncherMainFrame : public MainFrame {
public:
	LauncherMainFrame(std::shared_ptr<Resources> resources);

	virtual ~LauncherMainFrame() = default;

private:
	TaskManager _taskManager;
	QTimer _gameTimer;
	std::shared_ptr<Resources> _resources;

	LegendWidget* _legendWidget;
	ActionsWidget* _actionsWidget;
	ProgressWidget* _progressWidget;
	MapView* _mapView;
};
