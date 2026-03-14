#pragma once
#include <QObject>
#include <QUuid>
#include <list>
#include <memory>

class ItemPlacementStore;
class ItemMimeData;
class ItemPlacementService;
class InventoriesDataProvider;
class InventoryDataProvider;
class EquipmentDataProvider;
class ItemsService;

class InventoriesService : public QObject {
	Q_OBJECT
public:
	InventoriesService(
		InventoryDataProvider* inventoryDataProvider,
		EquipmentDataProvider* equipmentDataProvider,
		ItemsService* itemsService,
		QObject* parent = nullptr);

	~InventoriesService() override;

	bool addStore(const QUuid& id, std::unique_ptr<ItemPlacementStore> store);

	ItemPlacementService* placementService(const QUuid& id, bool loadIfNotExists) const;

	bool moveItem(const ItemMimeData& item, int col, int row, const QUuid& fromId, const QUuid& toId);

	/// Получить список всех загруженных инвентарей и экипировок
	std::list<QUuid> getAllIds() const;

signals:
	void itemMoved(const ItemMimeData& item, const QUuid& fromId, const QUuid& toId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
