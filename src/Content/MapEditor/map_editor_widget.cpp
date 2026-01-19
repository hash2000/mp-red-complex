#include "Content/MapEditor/map_editor_widget.h"
#include "Content/MapEditor/map_view.h"
#include "Content/MapEditor/map_control_panel.h"
#include <QSplitter>
#include <QHBoxLayout>

MapEditorWidget::MapEditorWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
	QWidget* parent)
: BaseTabWidget(resources, params, parent) {
	auto view = new MapView;
	auto panel = new MapControlPanel;
	auto splitter = new QSplitter(Qt::Horizontal);
	panel->resize(200, 0);
	splitter->addWidget(panel);
	splitter->addWidget(view);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	auto layout = new QHBoxLayout(this);
	layout->addWidget(splitter);
	setLayout(layout);
}
