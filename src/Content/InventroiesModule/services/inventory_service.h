#pragma once
#include "Content/InventroiesModule/services/items_placement_service.h"
#include "Libs/Base/container_view.h"
#include <QObject>
#include <QUuid>
#include <memory>

class InventoriesService;
class ItemsService;
class Inventory;
class InventoryItem;
class InventoryItemHandler;
class ItemMimeData;

class InventoryService : public IItemPlacementService {
	Q_OBJECT
public:
	InventoryService(ItemsService* itemsService, QObject* parent = nullptr);
	~InventoryService() override;

	/// Загрузить данные инвентаря
	bool load(const Inventory& inventory);

	QUuid placementId() const override;
	QString placementName() const override;

	int rows() const override;
	int cols() const override;

	int canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const override;
	std::optional<QPoint> findFreeSpace(const ItemMimeData& item, bool checkItemPlace) const override;

	bool moveItem(const ItemMimeData& item, int newCol, int newRow, bool checkItemPlace) override;
	void removeItem(const ItemMimeData& item) override;

	const InventoryItemHandler* itemById(const QUuid& id) const;
	const InventoryItemHandler* itemAt(int col, int row) const;

	bool containsItem(const ItemMimeData& item) const override;

	void clear() override;
	bool placeItem(const ItemMimeData& item);
	bool removeItemsFromStack(const ItemMimeData& item) override;

	ItemMimeData itemDataById(const QUuid& id) const override;

	void clearResourcesPermissions() override;

	void addResourcesPermissions(
		const std::list<ItemResourceType>& all,
		const std::list<ItemResourceType>& any) override;

signals:
	void placeItemEvent(const ItemMimeData& item, int col, int row);
	void removeItemEvent(const ItemMimeData& item, int col, int row);
	void moveItemEvent(const ItemMimeData& item, int col, int row, int newCol, int newRow);
	void itemCountChanged(const ItemMimeData& item);

private:
	class Private;
	std::unique_ptr<Private> d;

	friend class TestInventoriesService;  // Для юнит-тестов
};
