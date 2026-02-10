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

InventoryWindow::InventoryWindow(InventoryService* incentoryService, QWidget* parent)
: d(std::make_unique<Private>(this)) {
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
		if (args.size() < 2) {
			return false;
		}
		const auto target = QUuid::fromString(args.at(1));
		if (target.isNull()) {
			return false;
		}

		return d->widget->load(target);
	}

	return false;
}

