#include "Game/widgets/items/items_window.h"
#include "Game/widgets/items/items_widget.h"
#include "ApplicationLayer/items/items_service.h"

class ItemsWindow::Private {
public:
	Private(ItemsWindow* parent)
	: q(parent) {
	}

	ItemsWindow* q;
	ItemsService* service;
	ItemsWidget* widget;
};

ItemsWindow::ItemsWindow(ItemsService* service, const QString& id, QWidget* parent)
: d(std::make_unique<Private>(this))
, MdiChildWindow(id, parent) {
	d->service = service;
	d->widget = new ItemsWidget(service, this);
	setWidget(d->widget);
}

ItemsWindow::~ItemsWindow() = default;


bool ItemsWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		for (const auto& item : d->service->items()) {
			d->widget->addItemEntity(item);
		}

		return true;
	}

	return false;
}
