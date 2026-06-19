#pragma once
#include <QObject>
#include <memory>

class IEntitiesDataProvider;
class IItemsDataProvider;
class ImagesService;
class Item;
class ItemEntity;

class ItemsService : public QObject {
	Q_OBJECT
public:
	explicit ItemsService(
		ImagesService* imagesService,
		IEntitiesDataProvider* entitiesDataProvider,
		IItemsDataProvider* itemsDataProvider,
		QObject* parent = nullptr);
	~ItemsService() override;

	std::shared_ptr<Item> item(const QUuid& id);
	std::shared_ptr<Item> duplicate(const QUuid& id);
	std::shared_ptr<Item> createItemByEntity(const QUuid& entityId);
	std::shared_ptr<ItemEntity> entity(const QUuid& entityId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
