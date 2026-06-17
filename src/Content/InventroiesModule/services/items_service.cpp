#include "Content/InventroiesModule/services/items_service.h"
#include "Content/InventroiesModule/data_providers/i_items_data_provider.h"
#include "Content/InventroiesModule/data_providers/i_entities_data_provider.h"
#include "ApplicationLayer/textures/images_service.h"
#include <QDebug>
#include <map>

class ItemsService::Private {
public:
	Private(ItemsService* parent) : q(parent) { }
	ItemsService* q;

	IEntitiesDataProvider* entitiesDataProvider;
	IItemsDataProvider* itemsDataProvider;
	ImagesService* imagesService;
	std::unordered_map<QString, std::shared_ptr<ItemEntity>> entities;
};

ItemsService::ItemsService(
	IEntitiesDataProvider* entitiesDataProvider,
	IItemsDataProvider* itemsDataProvider,
	ImagesService* imagesService,
	QObject* parent)
	: d(std::make_unique<Private>(this))
	, QObject(parent) {
	d->entitiesDataProvider = entitiesDataProvider;
	d->itemsDataProvider = itemsDataProvider;
	d->imagesService = imagesService;
}

ItemsService::~ItemsService() = default;

std::shared_ptr<ItemEntity> ItemsService::entityById(const QString& entityId) const {
	const auto& it = d->entities.find(entityId);
	if (it != d->entities.end()) {
		return it->second;
	}

	auto entity = d->entitiesDataProvider->entity(entityId);
	if (!entity) {
		return nullptr;
	}

	if (!entity->iconPath.isEmpty()) {
		entity->icon = d->imagesService->getImage(entity->iconPath, ImageType::Entity);
	}

	d->entities[entityId] = entity;
	return entity;
}

std::shared_ptr<Item> ItemsService::itemById(const QUuid& id) {
	auto item = d->itemsDataProvider->item(id);
	if (!item) {
		return nullptr;
	}

	const auto entity = entityById(item->entityId);
	if (!entity) {
		return nullptr;
	}

	item->entity = entity;
	return item;
}

std::shared_ptr<Item> ItemsService::duplicate(const QUuid& id) {
	auto item = itemById(id);
	if (!item) {
		return std::shared_ptr<Item>();
	}

	item->id = QUuid::createUuid();
	if (!d->itemsDataProvider->save(*item)) {
		return std::shared_ptr<Item>();
	}

	return item;
}

std::shared_ptr<Item> ItemsService::createItemByEntity(const QString& entityId) {
	auto entity = entityById(entityId);
	if (!entity) {
		return std::shared_ptr<Item>();
	}

	auto item = std::make_shared<Item>();
	item->entityId = entityId;
	item->entity = entity;
	item->id = QUuid::createUuid();
	if (!d->itemsDataProvider->save(*item)) {
		return std::shared_ptr<Item>();
	}

	return item;
}
