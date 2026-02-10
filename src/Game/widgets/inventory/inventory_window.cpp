#include "Game/widgets/inventory/inventory_window.h"
#include "Game/widgets/inventory/inventory_grid_view.h"
#include "Game/widgets/inventory/inventory_grid.h"
#include <QVBoxLayout>
#include <QUuid>

class InventoryWindow::Private {
public:
	Private(InventoryWindow* parent)
		: q(parent) {
	}

	InventoryWindow* q;
	InventoryGridView* widget;
};

InventoryWindow::InventoryWindow(InventoryService* incentoryService, const QString& id, QWidget* parent)
: d(std::make_unique<Private>(this))
, MdiChildWindow(id, parent) {
	d->widget = new InventoryGridView(incentoryService, this);
	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(d->widget);

	setLayout(layout);
	setWindowTitle("Inventory");
}

InventoryWindow::~InventoryWindow() = default;

bool InventoryWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context)
{
	if (commandName == "create") {
		const auto target = QUuid::fromString(windowId());
		if (target.isNull()) {
			return false;
		}

		return d->widget->load(target);
	}

	return false;
}

