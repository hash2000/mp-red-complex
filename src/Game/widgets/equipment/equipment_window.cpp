#include "Game/widgets/equipment/equipment_window.h"
#include "Game/widgets/equipment/equipment_widget.h"
#include "Game/commands/command_context.h"
#include "Game/app_controller.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include <QVBoxLayout>
#include <QUuid>

class EquipmentWindow::Private {
public:
	Private(EquipmentWindow* parent)
		: q(parent) {
	}
	EquipmentWindow* q;
	EquipmentWidget* widget;
};

EquipmentWindow::EquipmentWindow(InventoriesService* inventoriesService, const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
	d->widget = new EquipmentWidget(inventoriesService, this);
	setWindowTitle("Equipment");
	setWidget(d->widget);
}

EquipmentWindow::~EquipmentWindow() = default;

EquipmentWidget* EquipmentWindow::widget() const {
	return d->widget;
}

bool EquipmentWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		const auto target = QUuid::fromString(windowId());
		if (target.isNull()) {
			return false;
		}

		if (!d->widget->setEquipmentService(target)) {
			return false;
		}

		connect(d->widget, &EquipmentWidget::containerOpened, this, [context](const ItemMimeData& item) {
			const auto itemIdStr = item.id
				.toString(QUuid::StringFormat::WithoutBraces)
				.toLower();
			context->applicationController()->executeCommandByName("window-create", QStringList{ "inventory", itemIdStr, item.name });
			});

		return true;
	}

	return false;
}
