#pragma once
#include "ApplicationLayer/equipment/equipment_item_handler.h"
#include <QFrame>
#include <memory>

class EquipmentSlot;
class EquipmentsService;
class InventoriesService;

class EquipmentWidget : public QFrame {
	Q_OBJECT

public:
	explicit EquipmentWidget(EquipmentsService* equipmentsService, InventoriesService* inventoriesService, QWidget* parent = nullptr);
	~EquipmentWidget() override;

	bool setEquipmentService(const QUuid& id);

private slots:
	void onItemEquipped(const EquipmentItemHandler& item, EquipmentSlotType slot, const QString& inventoryId);
	void onItemUnequipped(const EquipmentItemHandler& item, EquipmentSlotType slot);

private:
	class Private;
	std::unique_ptr<Private> d;
};
