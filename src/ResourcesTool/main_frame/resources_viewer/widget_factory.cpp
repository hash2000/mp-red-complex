#include "ResourcesTool/main_frame/resources_viewer/widget_factory.h"
#include "Content/MapEditor/map_editor_view.h"
#include "Content/ParticlesEditor/particles_editor_view.h"

WidgetsFactory::WidgetsFactory(const QVariantMap& params)
: _params(params) {
}

QWidget* WidgetsFactory::create(QWidget* parent) const {
	const auto type = _params.value("type").toString();

	if (type == "map_editor") {
		return new MapEditorView(_params, parent);
	}

	if (type == "particles_editor") {
		return new ParticlesEditorView(_params, parent);
	}

	return nullptr;
}
