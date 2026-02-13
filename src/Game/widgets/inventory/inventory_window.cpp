#include "Game/widgets/inventory/inventory_window.h"
#include "Game/widgets/inventory/inventory_grid_view.h"
#include "Game/widgets/inventory/inventory_grid.h"
#include <QVBoxLayout>
#include <QUuid>
#include <QMdiSubWindow>

class InventoryWindow::Private {
public:
	Private(InventoryWindow* parent)
		: q(parent) {
	}

	InventoryWindow* q;
	InventoryGridView* widget;
	QString inventoryName;
};

InventoryWindow::InventoryWindow(InventoriesService* inventoriesService, const QString& id, QWidget* parent)
: d(std::make_unique<Private>(this))
, MdiChildWindow(id, parent) {
	d->widget = new InventoryGridView(inventoriesService, this);
	d->inventoryName = "Inventory";
	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(d->widget);

	setLayout(layout);
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
