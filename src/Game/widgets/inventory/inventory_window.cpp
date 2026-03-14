#include "Game/widgets/inventory/inventory_window.h"
#include "Game/widgets/inventory/inventory_grid_view.h"
#include "Game/widgets/inventory/inventory_grid.h"
#include "ApplicationLayer/inventories_service.h"
#include <QVBoxLayout>
#include <QUuid>
#include <QMdiSubWindow>
#include <QEvent>
#include <QCloseEvent>

class InventoryWindow::Private {
public:
	Private(InventoryWindow* parent)
		: q(parent) {
	}

	InventoryWindow* q;
	InventoriesService* service;
	InventoryGridView* widget;
	QString inventoryName;
};

InventoryWindow::InventoryWindow(InventoriesService* service, const QString& id, QWidget* parent)
: d(std::make_unique<Private>(this))
, MdiChildWindow(id, parent) {
	d->service = service;
	d->widget = new InventoryGridView(service, this);
	d->inventoryName = "Inventory";
	setWidget(d->widget);
}

InventoryWindow::~InventoryWindow() = default;

bool InventoryWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context)
{
	if (commandName == "create") {
		const auto target = QUuid::fromString(windowId());
		if (target.isNull()) {
			return false;
		}

		if (!d->widget->load(target)) {
			return false;
		}

		d->inventoryName = d->widget->grid()->inventoryName();

		return true;
	}

	return false;
}

QString InventoryWindow::windowTitle() const {
	return d->inventoryName;
}

void InventoryWindow::closeEvent(QCloseEvent* closeEvent) {
	const auto target = d->widget->grid()->inventoryId();
	QMdiSubWindow::closeEvent(closeEvent);
}
