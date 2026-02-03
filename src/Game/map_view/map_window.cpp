#include "Game/map_view/map_window.h"
#include "Game/map_view/map_widget.h"
#include "Game/services/world_service.h"
#include "Game/event_bus/event_bus.h"
#include <QVBoxLayout>

class MapWindow::Private {
public:
	Private(MapWindow* parent)
		: q(parent) {
	}

	MapWindow* q;
	MapWidget* mapWidget;
	QVBoxLayout* layout;
};


MapWindow::MapWindow(WorldService* worldService, EventBus* eventBus, QWidget* parent)
	: MdiChildWindow(parent)
	, d(new Private(this)) {

	// Создаём внутренний виджет карты
	d->mapWidget = new MapWidget(this);

	d->layout = new QVBoxLayout(this);
	d->layout->setContentsMargins(0, 0, 0, 0);
	d->layout->addWidget(d->mapWidget);

	setLayout(d->layout);
	setWindowTitle("World Map");
}

MapWindow::~MapWindow() = default;

MapWidget* MapWindow::mapWidget() const {
	return d->mapWidget;
}

bool MapWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {

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
	return MdiChildWindow::handleCommand(commandName, args, context);
}

