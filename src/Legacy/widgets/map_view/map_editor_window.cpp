#include "Launcher/widgets/map_view/map_editor_window.h"
#include "Launcher/widgets/map_view/map_editor_widget.h"
#include "Launcher/services/time_service/time_service.h"
#include "Launcher/app_controller.h"
#include "Launcher/services.h"
#include "Launcher/commands/command_context.h"
#include "ApplicationLayer/maps/map_service.h"
#include "Content/TexturesModule/services/tiles_selector_service.h"
#include "Content/ShadersModule/services/shaders_service.h"
#include <memory>

class MapEditorWindow::Private {
public:
  Private(MapEditorWindow* parent) : q(parent) {}
  MapEditorWindow* q;

	TexturesService* textureService = nullptr;
  MapService* mapService = nullptr;
  TilesSelectorService* tilesSelectorService = nullptr;
  TimeService* timeService = nullptr;
  MapEditorWidget* editorWidget = nullptr;
	std::unique_ptr<ShadersService> shadersService;
};

MapEditorWindow::MapEditorWindow(const QString& id, QWidget* parent)
  : MdiChildWindow(id, parent)
  , d(std::make_unique<Private>(this)) {
}

MapEditorWindow::~MapEditorWindow() = default;


bool MapEditorWindow::handleCommand(const QString& commandName,
  const QStringList& args,
  CommandContext* context) {
	if (commandName == "create") {
		auto services = context->services();
		d->shadersService = std::move(services->shadersService());
		d->textureService = services->texturesService();
		d->mapService = services->mapService();
		d->tilesSelectorService = services->tilesSelectorService();
		d->timeService = services->timeService();

		d->editorWidget = new MapEditorWidget(
			d->shadersService.get(),
			d->textureService,
			d->mapService,
			d->tilesSelectorService,
			this);

		setWidget(d->editorWidget);

		// Подписываемся на тики для обновления
		if (d->timeService) {
			connect(d->timeService, &TimeService::tick,	d->editorWidget, QOverload<>::of(&MapEditorWidget::update));
		}

		return true;
	}

	return false;
}
