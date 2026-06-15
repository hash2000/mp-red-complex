#include "Content/InventroiesModule/services/items_service.h"
#include "Content/InventroiesModule/data_providers/i_items_data_provider.h"

#include "ApplicationLayer/textures/images_service.h"
#include <QDebug>
#include <map>

class ItemsService::Private {
public:
	Private(ItemsService* parent) : q(parent) { }
	ItemsService* q;

	IItemsDataProvider* itemsDataProvider;
	ImagesService* imagesService;
	std::map<QString, std::unique_ptr<ItemEntity>> itemEntities;
	std::map<QUuid, std::unique_ptr<Item>> items;
};

ItemsService::ItemsService(
	IItemsDataProvider* itemsDataProvider,
	ImagesService* imagesService,
	QObject* parent)
	: d(std::make_unique<Private>(this))
	, QObject(parent) {
	d->itemsDataProvider = itemsDataProvider;
	d->imagesService = imagesService;
}

ItemsService::~ItemsService() = default;


ItemsService::EntityView ItemsService::entities() const {
	return make_deref_view(d->itemEntities);
}

ItemsService::ItemView ItemsService::items() const {
	return make_deref_view(d->items);
}

const ItemEntity* ItemsService::entityById(const QString& entityId) const {
	return nullptr;
}

const Item* ItemsService::itemById(const QUuid& id) {
	return nullptr;
}


const Item* ItemsService::duplicate(const QUuid& id) {
	return nullptr;
}

const Item* ItemsService::createItemByEntity(const QString& entityId) {
	return nullptr;
}
