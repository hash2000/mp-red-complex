#pragma once
#include <QObject>
#include <QUuid>
#include <list>
#include <memory>

class ItemMimeData;
class ItemPlacementService;
class InventoriesDataProvider;
class ItemsService;
struct PlacementCreateParams;

class InventoriesService : public QObject {
	Q_OBJECT
public:
	InventoriesService(
		ItemsService* itemsService,
		QObject* parent = nullptr);

	~InventoriesService() override;

	/// Получить сервис размещения по идентификатору
	/// @param id Идентификатор инвентаря/экипировки
	/// @param loadIfNotExists Загружать если ещё не загружен
	/// @return Сервис размещения или nullptr если не найден
	ItemPlacementService* placementService(const QUuid& id, bool loadIfNotExists) const;

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
