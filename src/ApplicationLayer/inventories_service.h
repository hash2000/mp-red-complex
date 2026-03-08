#pragma once
#include <QObject>
#include <QUuid>
#include <memory>

class InventoryLoader;
class ItemMimeData;
class ItemPlacementService;

class InventoriesService : public QObject {
	Q_OBJECT
public:
	InventoriesService(QObject* parent = nullptr);
	~InventoriesService() override;

	ItemPlacementService* placementService(const QUuid& id, bool loadIfNotExists) const;

	bool applyLoader(const QUuid& id, std::unique_ptr<InventoryLoader> loader);

	bool moveItem(const ItemMimeData& item, int col, int row, const QUuid& fromId, const QUuid& toId);

signals:
	void itemMoved(const ItemMimeData& item, const QUuid& fromId, const QUuid& toId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
