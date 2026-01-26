#include "Launcher/main_frame.h"
#include "Launcher/widgets/legend_widget.h"
#include "Launcher/widgets/actions_widget.h"
#include "Launcher/map/map_view.h"
#include <QSplitter>
#include <QTabWidget>

LauncherMainFrame::LauncherMainFrame(std::shared_ptr<Resources> resources)
: _resources(resources) {
	auto legend = new LegendWidget;
	auto actions = new ActionsWidget;

	auto splitter = new QSplitter;
	auto mapView = new MapView(actions);
	auto tabs = new QTabWidget;

	QObject::connect(mapView, &MapView::tileInDirectionChanged,
		actions, &ActionsWidget::onTileInDirectionChanged);

	tabs->addTab(legend, "Legend");
	tabs->addTab(actions, "Actions");

	splitter->addWidget(mapView);
	splitter->addWidget(tabs);
	splitter->addWidget(tabs);
	splitter->setSizes({ 800, 200 });

	setCentralWidget(splitter);
}
