#include "Game/widgets/map_view/map_window.h"
#include "Game/widgets/map_view/map_widget.h"
#include "Game/services/world_service/world_service.h"
#include "Game/services/time_service/time_service.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/commands/command_context.h"
#include "ApplicationLayer/shaders/shaders_service.h"

class MapWindow::Private {
public:
	Private(MapWindow* parent) : q(parent) { }
	MapWindow* q;

	std::unique_ptr<ShadersService> shadersService;
};


MapWindow::MapWindow(const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {
}

MapWindow::~MapWindow() = default;

bool MapWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		auto controller = context->applicationController();
		auto services = controller->services();

		d->shadersService = std::move(services->shadersService());
		auto widget = new MapWidget(
			d->shadersService.get(),
			services->tilesSelectorService(),
			this);

		setWindowTitle("World Map");

		connect(services->timeService(), &TimeService::tick, widget, &MapWidget::onTick);

		setWidget(widget);

		return true;
	}

	//if (commandName == "center") {
	//	// Пример команды: center player
	//	if (args.value(0) == "player") {
	//		d->mapWidget->centerOnPlayer();
	//		context->printSuccess("Map centered on player");
	//		return true;
	//	}
	//	// Другие варианты центрирования...
	//	return false;
	//}

	//if (commandName == "zoom") {
	//	bool ok = false;
	//	double factor = args.value(0).toDouble(&ok);
	//	if (ok && factor > 0) {
	//		d->mapWidget->setZoom(factor);
	//		context->printSuccess(QString("Zoom set to %1x").arg(factor));
	//		return true;
	//	}
	//	context->printError("Usage: zoom <factor>");
	//	return false;
	//}

	// Не обработано — передать базовому классу или выше
	return false;
}

