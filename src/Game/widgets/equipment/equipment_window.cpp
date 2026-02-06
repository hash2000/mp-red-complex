#include "Game/widgets/equipment/equipment_window.h"
#include "Game/widgets/equipment/equipment_widget.h"
#include "Game/services/inventory_service/inventory_service.h"
#include <QVBoxLayout>

class EquipmentWindow::Private {
public:
	Private(EquipmentWindow* parent)
		: q(parent) {
	}
	EquipmentWindow* q;
	EquipmentWidget* widget;
};

EquipmentWindow::EquipmentWindow(InventoryService* incentoryService, QWidget* parent)
	: d(std::make_unique<Private>(this)) {
	d->widget = new EquipmentWidget(this);
	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(d->widget);

	setLayout(layout);
	setWindowTitle("Equipment");

	connect(d->widget, &EquipmentWidget::itemEquipped, [](const Item& item, EquipmentSlot::SlotType slot) {
		qDebug() << "Equipped" << item.name << "to slot" << static_cast<int>(slot);
		});

	connect(d->widget, &EquipmentWidget::itemUnequipped, [](const Item& item, EquipmentSlot::SlotType slot) {
		qDebug() << "Unequipped" << item.name << "from slot" << static_cast<int>(slot);
		});

	Item sword{ "sword_001", "Rusty Sword", Item::Type::Weapon, QPixmap(":/icons/sword.png"), 1 };
	d->widget->equipItem(sword);
}

EquipmentWindow::~EquipmentWindow() = default;

EquipmentWidget* EquipmentWindow::mapWidget() const {
	return d->widget;
}

bool EquipmentWindow::handleCommand(const QString& commandName,
	const QStringList& args,
	CommandContext* context) {

	return MdiChildWindow::handleCommand(commandName, args, context);
}
