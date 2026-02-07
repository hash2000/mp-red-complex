#include "Game/widgets/inventory/inventory_window.h"
#include "Game/widgets/inventory/inventory_grid_view.h"
#include "Game/services/inventory_service/inventory_service.h"
#include <QVBoxLayout>

class InventoryWindow::Private {
public:
	Private(InventoryWindow* parent)
		: q(parent) {
	}

	InventoryWindow* q;
	InventoryService* incentoryService;
	InventoryGridView* widget;
};

InventoryWindow::InventoryWindow(InventoryService* incentoryService, QWidget* parent)
: d(std::make_unique<Private>(this)) {
	d->incentoryService = incentoryService;
	d->widget = new InventoryGridView(100, 10, this);
	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(d->widget);

	setLayout(layout);
	setWindowTitle("Inventory");
}

InventoryWindow::~InventoryWindow() = default;

bool InventoryWindow::handleCommand(const QString& commandName,
	const QStringList& args,
	CommandContext* context)
{

	return false;
}

