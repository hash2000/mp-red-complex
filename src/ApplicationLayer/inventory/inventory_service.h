#pragma once
#include "ApplicationLayer/i_items_placement_service.h"
#include "Base/container_view.h"
#include <QObject>
#include <QUuid>
#include <memory>

class InventoriesService;
class ItemsService;
class Inventory;
class InventoryItem;
class InventoryItemHandler;
class ItemMimeData;

class InventoryService : public QObject, public IItemPlacementService {
	Q_OBJECT
public:
	InventoryService(ItemsService* itemsService);
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

	using EntityView = decltype(make_deref_view(std::declval<const std::map<QUuid, std::unique_ptr<InventoryItemHandler>>&>()));
	EntityView items() const;

	bool containsItem(const ItemMimeData& item) const override;

	void clear() override;
	bool placeItem(const ItemMimeData& item);
	bool removeItemsFromStack(const ItemMimeData& item) override;

	ItemMimeData itemDataById(const QUuid& id) const override;

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
