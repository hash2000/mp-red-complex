#include "Launcher/widgets/inventory/inventory_window.h"
//#include "Launcher/widgets/inventory/inventory_grid_view.h"
//#include "Launcher/widgets/inventory/inventory_grid.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/app_controller.h"
#include "Launcher/services.h"
//#include "Content/InventoriesModule/services/inventories_service.h"
#include "Content/InventoriesModule/models/item_mime_data.h"
#include <QVBoxLayout>
#include <QUuid>
#include <QMdiSubWindow>
#include <QEvent>
#include <QCloseEvent>

class InventoryWindow::Private {
public:
	Private(InventoryWindow* parent) : q(parent) { }
	InventoryWindow* q;

	//InventoryGridView* widget = nullptr;
	QString inventoryName;
};

InventoryWindow::InventoryWindow(const QString& id, QWidget* parent)
: d(std::make_unique<Private>(this))
, MdiChildWindow(id, parent) {	
}

InventoryWindow::~InventoryWindow() = default;

bool InventoryWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context)
{
	if (commandName == "create") {
		//const auto target = QUuid::fromString(windowId());
		//if (target.isNull()) {
		//	return false;
		//}

		//auto services = context->services();
		//auto controller = context->applicationController();
		//d->widget = new InventoryGridView(services->inventoriesService(), this);
		//setWidget(d->widget);

		//if (!d->widget->load(target)) {
		//	return false;
		//}

		//d->inventoryName = d->widget->grid()->inventoryName();

		//connect(d->widget->grid(), &InventoryGrid::containerOpened, this, [controller](const ItemMimeData& item) {
		//	const auto itemIdStr = QString("id:%1")
		//		.arg(item.id
		//			.toString(QUuid::StringFormat::WithoutBraces)
		//			.toLower());

		//	controller->executeCommandByName("window-create", QStringList{ "target:inventory", itemIdStr });
		//	});

		return true;
	}

	return false;
}

QString InventoryWindow::windowTitle() const {
	return d->inventoryName;
}

void InventoryWindow::closeEvent(QCloseEvent* closeEvent) {
	//const auto target = d->widget->grid()->inventoryId();
	//QMdiSubWindow::closeEvent(closeEvent);
}
