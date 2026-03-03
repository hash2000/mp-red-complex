#pragma once
#include "ApplicationLayer/equipment/equipment_item_handler.h"
#include <QFrame>
#include <memory>

class EquipmentSlot;
class EquipmentsService;

class EquipmentWidget : public QFrame {
	Q_OBJECT

public:
	explicit EquipmentWidget(EquipmentsService* equipmentsService, QWidget* parent = nullptr);
	~EquipmentWidget() override;

	bool setInventoryService(const QUuid& id);

private slots:
	void onItemEquipped(const EquipmentItemHandler& item, EquipmentSlotType slot);
	void onItemUnequipped(const EquipmentItemHandler& item, EquipmentSlotType slot);

private:
	class Private;
	std::unique_ptr<Private> d;
};
