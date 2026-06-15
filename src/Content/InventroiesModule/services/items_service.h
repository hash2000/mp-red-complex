#pragma once
#include "Libs/Base/container_view.h"
#include "Content/InventroiesModule/models/item.h"
#include <QObject>
#include <QString>
#include <memory>
#include <map>

class ImagesService;
class IItemsDataProvider;

class ItemsService : public QObject {
	Q_OBJECT
public:
	explicit ItemsService(
		IItemsDataProvider* itemsDataProvider,
		ImagesService* imagesService,
		QObject* parent = nullptr);
	~ItemsService() override;

	using EntityView = decltype(make_deref_view(std::declval<const std::map<QString, std::unique_ptr<ItemEntity>>&>()));
	EntityView entities() const;

	using ItemView = decltype(make_deref_view(std::declval<const std::map<QUuid, std::unique_ptr<Item>>&>()));
	ItemView items() const;

	const ItemEntity* entityById(const QString& entityId) const;
	const Item* itemById(const QUuid& id);
	const Item* duplicate(const QUuid& id);
	const Item* createItemByEntity(const QString& entityId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
