#include "Game/widgets/equipment/equipment_window.h"
#include "Game/widgets/equipment/equipment_widget.h"
#include "Game/commands/command_context.h"
#include "Game/app_controller.h"
#include "Game/services.h"

#include "ApplicationLayer/items/item_mime_data.h"
#include <QVBoxLayout>
#include <QUuid>

class EquipmentWindow::Private {
public:
	Private(EquipmentWindow* parent) : q(parent) { }
	EquipmentWindow* q;

	EquipmentWidget* widget;
};

EquipmentWindow::EquipmentWindow(const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
}

EquipmentWindow::~EquipmentWindow() = default;

bool EquipmentWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		const auto target = QUuid::fromString(windowId());
		if (target.isNull()) {
			return false;
		}

		auto controller = context->applicationController();
		auto services = controller->services();

		d->widget = new EquipmentWidget(services->inventoriesService(), this);
		setWindowTitle("Equipment");
		setWidget(d->widget);

		if (!d->widget->setEquipmentService(target)) {
			return false;
		}

		connect(d->widget, &EquipmentWidget::containerOpened, this, [controller](const ItemMimeData& item) {
			const auto itemIdStr = item.id
				.toString(QUuid::StringFormat::WithoutBraces)
				.toLower();
			controller->executeCommandByName("window-create", QStringList{ "inventory", itemIdStr, item.name });
			});

		return true;
	}

	return false;
}
