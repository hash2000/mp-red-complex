#include "Game/widgets/map_view/map_window.h"
#include "Game/widgets/map_view/map_widget.h"
#include "Game/services/world_service/world_service.h"
#include "Game/services/time_service/time_service.h"
#include <QVBoxLayout>

class MapWindow::Private {
public:
	Private(MapWindow* parent)
		: q(parent) {
	}

	MapWindow* q;
	MapWidget* mapWidget;
};


MapWindow::MapWindow(WorldService* worldService, TimeService* timeService, QWidget* parent)
	: MdiChildWindow(parent)
	, d(std::make_unique<Private>(this)) {

	// Создаём внутренний виджет карты
	d->mapWidget = new MapWidget(this);
	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(d->mapWidget);

	setLayout(layout);
	setWindowTitle("World Map");

	connect(timeService, &TimeService::tick, d->mapWidget, &MapWidget::onTick);
}

MapWindow::~MapWindow() = default;

MapWidget* MapWindow::widget() const {
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

