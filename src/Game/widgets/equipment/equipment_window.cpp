#include "Game/widgets/equipment/equipment_window.h"
#include "Game/widgets/equipment/equipment_widget.h"
#include <QVBoxLayout>

class EquipmentWindow::Private {
public:
	Private(EquipmentWindow* parent)
		: q(parent) {
	}
	EquipmentWindow* q;
	EquipmentWidget* widget;
	EquipmentService* equipmentService;
};

EquipmentWindow::EquipmentWindow(EquipmentService* equipmentService, const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
	d->widget = new EquipmentWidget(equipmentService, this);
	setWindowTitle("Equipment");

	//connect(d->widget, &EquipmentWidget::itemEquipped, [](const EquipmentItem& item, EquipmentSlotType slot) {
	//	qDebug() << "Equipped" << item.entity->name << "to slot" << static_cast<int>(slot);
	//	});

	//connect(d->widget, &EquipmentWidget::itemUnequipped, [](const EquipmentItem& item, EquipmentSlotType slot) {
	//	qDebug() << "Unequipped" << item.entity->name << "from slot" << static_cast<int>(slot);
	//	});

	setWidget(d->widget);
}

EquipmentWindow::~EquipmentWindow() = default;

EquipmentWidget* EquipmentWindow::widget() const {
	return d->widget;
}

bool EquipmentWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {

		return true;
	}

	return false;
}
