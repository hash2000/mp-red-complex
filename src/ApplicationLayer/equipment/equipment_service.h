#pragma once
#include "ApplicationLayer/items_placement_service.h"
#include <QObject>
#include <memory>

class ItemsService;
class Equipment;
class EquipmentItemHandler;
class ItemMimeData;
enum class EquipmentSlotType;

class EquipmentService : public QObject, public ItemPlacementService {
	Q_OBJECT
public:
	EquipmentService(ItemsService* itemsService, QObject* parent = nullptr);
	~EquipmentService() override;

	bool load(const Equipment& equipment);

	QString placementId() const override;

	int canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const override;
	std::optional<QPoint> findFreeSpace(const ItemMimeData& item, bool checkItemPlace) const override;

	bool containsItem(const ItemMimeData& item) const override;

	bool moveItem(const ItemMimeData& item, int newCol, int newRow, bool checkItemPlace) override;
	void removeItem(const ItemMimeData& item) override;

	int rows() const override;
	int cols() const override;

	void clear() override;

	bool applyDublicateFromItem(const ItemMimeData& item) override;
	bool removeItemsFromStack(const ItemMimeData& item) override;


	const EquipmentItemHandler* equipItem(const ItemMimeData& item, EquipmentSlotType slot, const QString& inventoryId);
	bool unequipItem(const ItemMimeData& item, EquipmentSlotType slot);

	const EquipmentItemHandler* itemBySlot(EquipmentSlotType slot) const;

	using ItemsMap = const std::map<EquipmentSlotType, std::unique_ptr<EquipmentItemHandler>>&;
	ItemsMap items() const;

signals:
	void itemEquipped(const EquipmentItemHandler& item, EquipmentSlotType slot, const QString& inventoryId);
	void itemUnequipped(const EquipmentItemHandler& item, EquipmentSlotType slot);
	void removeItemEvent(const ItemMimeData& item, int row, int col);

private:
	class Private;
	std::unique_ptr<Private> d;
};
