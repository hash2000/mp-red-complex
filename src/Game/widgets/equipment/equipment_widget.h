#pragma once
#include "ApplicationLayer/equipment/equipment_item_handler.h"
#include <QFrame>
#include <QUuid>
#include <memory>

class EquipmentSlot;
class InventoriesService;
class ItemMimeData;

class EquipmentWidget : public QFrame {
	Q_OBJECT

public:
	explicit EquipmentWidget(InventoriesService* inventoriesService, QWidget* parent = nullptr);
	~EquipmentWidget() override;

	bool setEquipmentService(const QUuid& id);

private slots:
	void onItemEquipped(const EquipmentItemHandler& item, EquipmentSlotType slot, const QUuid& inventoryId);
	void onItemUnequipped(const EquipmentItemHandler& item, EquipmentSlotType slot);

signals:
	void containerOpened(const ItemMimeData& container);

private:
	class Private;
	std::unique_ptr<Private> d;
};
