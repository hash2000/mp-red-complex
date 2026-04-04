#include "Game/widgets/map_view/map_window.h"
#include "Game/widgets/map_view/map_widget.h"
#include "Game/services/world_service/world_service.h"
#include "Game/services/time_service/time_service.h"
#include <QVBoxLayout>
#include <QDebug>

class MapWindow::Private {
public:
	Private(MapWindow* parent)
		: q(parent) {
	}

	MapWindow* q;
	MapWidget* widget;
};


MapWindow::MapWindow(WorldService* worldService, TimeService* timeService, const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {
	// Создаём внутренний виджет карты
	d->widget = new MapWidget(this);

	setWindowTitle("World Map");

	connect(timeService, &TimeService::tick, d->widget, &MapWidget::onTick);

	setWidget(d->widget);
}

MapWindow::~MapWindow() = default;

MapWidget* MapWindow::widget() const {
	return d->widget;
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
	return true;
}

