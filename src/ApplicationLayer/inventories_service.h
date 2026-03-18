#pragma once
#include <QObject>
#include <QUuid>
#include <list>
#include <memory>

class ItemMimeData;
class IItemPlacementService;
class InventoriesDataProvider;
class ItemsService;
class InventoryLoader;
struct PlacementCreateParams;

class InventoriesService : public QObject {
	Q_OBJECT
public:
	InventoriesService(
		ItemsService* itemsService,
		InventoryLoader* inventoryLoader,
		QObject* parent = nullptr);

	~InventoriesService() override;

	/// Получить сервис размещения по идентификатору
	/// @param id Идентификатор инвентаря/экипировки
	/// @param loadIfNotExists Загружать если ещё не загружен
	/// @return Сервис размещения или nullptr если не найден
	IItemPlacementService* placementService(const QUuid& id, bool loadIfNotExists) const;

	/// Получить все идентификаторы загруженных инвентарей/экипировок
	std::vector<QUuid> loadedPlacementIds() const;

	/// Переместить предмет между инвентарями
	bool moveItem(const ItemMimeData& item, int col, int row, const QUuid& fromId, const QUuid& toId);

signals:
	void itemMoved(const ItemMimeData& item, const QUuid& fromId, const QUuid& toId);
	void inventoryCreated(const QUuid& id);
	void equipmentCreated(const QUuid& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
