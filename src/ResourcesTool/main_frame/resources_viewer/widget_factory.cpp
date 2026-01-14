#include "ResourcesTool/main_frame/resources_viewer/widget_factory.h"
#include "Content/MapEditor/map_editor_view.h"
#include "Content/ParticlesEditor/particles_editor_view.h"
#include "Content/ShaderEditor/shaders_editor_view.h"

WidgetsFactory::WidgetsFactory(const QVariantMap& params)
: _params(params) {
}

BaseTabWidget* WidgetsFactory::create(QWidget* parent) const {
	auto baseWidget = createBaseWidget();
	if (!baseWidget) {
		return nullptr;
	}

	return new BaseTabWidgetWrapper(baseWidget, parent);
}

QWidget* WidgetsFactory::createBaseWidget() const
{
	const auto type = _params.value("type").toString();

	if (type == "map_editor") {
		return new MapEditorView(_params);
	}

	if (type == "particles_editor") {
		return new ParticlesEditorView(_params);
	}

	if (type == "shaders_editor") {
		return new ShadersEditorView(_params);
	}

	return nullptr;
}
