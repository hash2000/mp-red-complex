#include "Content/MapEditor/map_editor_view.h"

MapEditorView::MapEditorView(std::shared_ptr<Resources> resources, const QVariantMap& params,
	QWidget* parent)
: BaseTabWidget(resources, params, parent) {
}
