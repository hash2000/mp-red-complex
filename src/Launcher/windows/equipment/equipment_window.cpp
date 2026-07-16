#include "Launcher/windows/equipment/equipment_window.h"
//#include "Launcher/windows/equipment/equipment_widget.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Launcher/app_controller.h"
#include "Launcher/services.h"

#include "Content/InventoriesModule/models/item_mime_data.h"
#include <QVBoxLayout>
#include <QUuid>

class EquipmentWindow::Private {
public:
	Private(EquipmentWindow* parent) : q(parent) { }
	EquipmentWindow* q;

//	EquipmentWidget* widget;
};

EquipmentWindow::EquipmentWindow(const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
}

EquipmentWindow::~EquipmentWindow() = default;

bool EquipmentWindow::handleCommand(const std::shared_ptr<Instruction> cmd, CommandContext* context) {
	const auto action = cmd->parameter("action")
		.toString();
	if (action == "create") {
		//const auto target = QUuid::fromString(windowId());
		//if (target.isNull()) {
		//	return false;
		//}

		//auto services = context->services();
		//auto controller = context->applicationController();

		//d->widget = new EquipmentWidget(services->inventoriesService(), this);
		//setWindowTitle("Equipment");
		//setWidget(d->widget);

		//if (!d->widget->setEquipmentService(target)) {
		//	return false;
		//}

		//connect(d->widget, &EquipmentWidget::containerOpened, this, [controller](const ItemMimeData& item) {
		//	const auto itemIdStr = QString("id:%1")
		//		.arg(item.id
		//		.toString(QUuid::StringFormat::WithoutBraces)
		//		.toLower());

		//	controller->executeCommandByName("window-create", QStringList{ "target:inventory", itemIdStr, item.name });
		//	});

		return true;
	}

	return false;
}
