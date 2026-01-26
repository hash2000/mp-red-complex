#include "Launcher/main_frame.h"
#include <QSplitter>
#include <QTabWidget>

LauncherMainFrame::LauncherMainFrame(std::shared_ptr<Resources> resources)
: _resources(resources) {
	_legendWidget = new LegendWidget;
	_actionsWidget = new ActionsWidget;
	_progressWidget = new ProgressWidget;

	auto splitter = new QSplitter;
	auto tabs = new QTabWidget;
	_mapView = new MapView(&_taskManager, _actionsWidget);

	QObject::connect(_mapView, &MapView::tileInDirectionChanged, _actionsWidget, &ActionsWidget::onTileInDirectionChanged);
	QObject::connect(_mapView, &MapView::nextPerformDig, _mapView, &MapView::performDig);
	QObject::connect(_actionsWidget, &ActionsWidget::centerOnPlayerRequested, _mapView, &MapView::centerOnPlayerAnimated);

	QObject::connect(_actionsWidget, &ActionsWidget::digRequested, _mapView, &MapView::performDig);

	QObject::connect(&_taskManager, &TaskManager::taskAdded, _progressWidget, &ProgressWidget::onTaskAdded);
	QObject::connect(&_taskManager, &TaskManager::taskUpdated, _progressWidget, &ProgressWidget::onTaskUpdated);
	QObject::connect(&_taskManager, &TaskManager::taskCompleted, _progressWidget, &ProgressWidget::onTaskCompleted);

	tabs->addTab(_actionsWidget, "Actions");
	tabs->addTab(_progressWidget, "Progress");
	tabs->addTab(_legendWidget, "Legend");

	splitter->addWidget(_mapView);
	splitter->addWidget(tabs);
	splitter->addWidget(tabs);
	splitter->setSizes({ 800, 200 });

	setCentralWidget(splitter);

	_gameTimer.setInterval(30); // ~33 FPS
	connect(&_gameTimer, &QTimer::timeout, &_taskManager, &TaskManager::update);
	_gameTimer.start();
}
