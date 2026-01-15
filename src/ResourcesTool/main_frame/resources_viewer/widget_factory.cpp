#include "ResourcesTool/main_frame/resources_viewer/widget_factory.h"
#include "Content/MapEditor/map_editor_view.h"
#include "Content/ParticlesEditor/particles_editor_view.h"
#include "Content/ShaderEditor/shaders_editor_widget.h"
#include "ResourcesTool/widgets/animation/atlas_widget.h"
#include "ResourcesTool/widgets/hex/hex_dump_widget.h"
#include "ResourcesTool/widgets/procedure/procedure_explorer_widget.h"
#include "ResourcesTool/widgets/messages/messages_explorer_widget.h"
#include "ResourcesTool/widgets/text/text_widget.h"
#include "ResourcesTool/widgets/pallete/pallete_explorer_widget.h"

WidgetsFactory::WidgetsFactory(std::shared_ptr<Resources> resources, const QVariantMap& params)
: _params(params)
, _resources(resources) {
}

BaseTabWidget* WidgetsFactory::create(QWidget* parent) const {
	if (_params.contains("type")) {
		const auto type = _params.value("type").toString();
		auto baseWidget = createExternalWidget(type);
		if (baseWidget) {
			return new BaseTabWidgetWrapper(_resources, _params, baseWidget, parent);
		}
	}

	if (_params.contains("suffix")) {
		const auto suffix = _params.value("suffix").toString();
		const auto typeOpt = From<WidgetResource>::from(suffix);
		if (!typeOpt) {
			qWarning() << "Unknown suffix widget requested";
			return nullptr;
		}

		const auto type = typeOpt.value();
		return createResourceTypeWidget(type, parent);
	}

	qWarning() << "Unknown type widget requested";

	return nullptr;
}

QWidget* WidgetsFactory::createExternalWidget(const QString& type) const
{
	if (type == "map_editor") {
		return new MapEditorView(_params);
	}
	if (type == "particles_editor") {
		return new ParticlesEditorView(_params);
	}
	if (type == "shaders_editor") {
		return new ShadersEditorWidget(_resources, _params);
	}

	return nullptr;
}

BaseTabWidget* WidgetsFactory::createResourceTypeWidget(WidgetResource type, QWidget* parent) const
{
	switch (type) {
	case WidgetResource::Frm: return new AtlasWidget(_resources, _params, parent);
	case WidgetResource::Hex: return new HexDumpWidget(_resources, _params, parent);
	case WidgetResource::Int: return new ProcedureExplorerWidget(_resources, _params, parent);
	case WidgetResource::Msg: return new MessagesExplorerWidget(_resources, _params, parent);
	case WidgetResource::Pal: return new PalleteExplorerWidget(_resources, _params, parent);
	case WidgetResource::Text: return new TextWidget(_resources, _params, parent);
	case WidgetResource::Bio: return new TextWidget(_resources, _params, parent);
	}

	return nullptr;
}
