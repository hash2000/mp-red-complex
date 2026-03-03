#pragma once
#include <QObject>
#include <memory>

class ItemsService;
class Equipment;
class EquipmentItemHandler;
class ItemMimeData;
enum class EquipmentSlotType;

class EquipmentService : public QObject {
	Q_OBJECT
public:
	EquipmentService(ItemsService* itemsService, QObject* parent = nullptr);
	~EquipmentService() override;

	bool load(const Equipment& equipment);

	bool canAcceptItem(const ItemMimeData& item, EquipmentSlotType slot) const;

	const EquipmentItemHandler* equipItem(const ItemMimeData& item, EquipmentSlotType slot);
	bool unequipItem(const ItemMimeData& item, EquipmentSlotType slot);

	const EquipmentItemHandler* itemBySlot(EquipmentSlotType slot) const;

signals:
	void itemEquipped(const EquipmentItemHandler& item, EquipmentSlotType slot);
	void itemUnequipped(const EquipmentItemHandler& item, EquipmentSlotType slot);


private:
	class Private;
	std::unique_ptr<Private> d;
};
