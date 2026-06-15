#pragma once
#include "Libs/Base/container_view.h"
#include "Content/InventroiesModule/models/item.h"
#include <QObject>
#include <QString>
#include <memory>
#include <map>

class ImagesService;
class IItemsDataProvider;
class IEntitiesDataProvider;

class ItemsService : public QObject {
	Q_OBJECT
public:
	explicit ItemsService(
		IEntitiesDataProvider* entitiesDataProvider,
		IItemsDataProvider* itemsDataProvider,
		ImagesService* imagesService,
		QObject* parent = nullptr);
	~ItemsService() override;

	using EntityView = decltype(make_deref_view(std::declval<const std::map<QString, std::unique_ptr<ItemEntity>>&>()));
	EntityView entities() const;

	const ItemEntity* entityById(const QString& entityId) const;
	std::shared_ptr<Item> itemById(const QUuid& id);
	std::shared_ptr<Item> duplicate(const QUuid& id);
	std::shared_ptr<Item> createItemByEntity(const QString& entityId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
