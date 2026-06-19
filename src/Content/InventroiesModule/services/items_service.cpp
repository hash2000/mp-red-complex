#include "Content/InventroiesModule/services/items_service.h"
#include "Content/InventroiesModule/data_providers/i_items_data_provider.h"
#include "Content/InventroiesModule/data_providers/i_entities_data_provider.h"
#include "Content/InventroiesModule/models/item.h"

#include "ApplicationLayer/textures/images_service.h"

#include <QDebug>
#include <map>

class ItemsService::Private {
public:
	Private(ItemsService* parent) : q(parent) {}
	ItemsService* q;

	ImagesService* imagesService;
	IEntitiesDataProvider* entitiesDataProvider;
	IItemsDataProvider* itemsDataProvider;
	std::map<QUuid, std::shared_ptr<ItemEntity>> entities;

	void loadEntitieas();
};

ItemsService::ItemsService(
	ImagesService* imagesService,
	IEntitiesDataProvider* entitiesDataProvider,
	IItemsDataProvider* itemsDataProvider,
	QObject* parent)
	: d(std::make_unique<Private>(this))
	, QObject(parent) {
	d->imagesService = imagesService;
	d->itemsDataProvider = itemsDataProvider;
	d->entitiesDataProvider = entitiesDataProvider;

	d->loadEntitieas();
}

ItemsService::~ItemsService() = default;

void ItemsService::Private::loadEntitieas() {
	entities.clear();
	auto list = entitiesDataProvider->entities();
	for (auto i : list) {
		if (!i->iconPath.isEmpty()) {
			i->icon = imagesService->getImage(i->iconPath, ImageType::Item);
		}

		entities.emplace(i->id, i);
	}
}

std::shared_ptr<Item> ItemsService::item(const QUuid& id) {
	auto ptr = d->itemsDataProvider->item(id);
	if (!ptr) {
		return std::shared_ptr<Item>();
	}

	if (ptr->entityId.isNull()) {
		qWarning() << "Load item" << id << ". Undefined entity";
		return std::shared_ptr<Item>();
	}

	const auto& it = d->entities.find(ptr->entityId);
	if (it == d->entities.end()) {
		qWarning() << "Load item" << id << ". Entity is unknown";
		return std::shared_ptr<Item>();
	}

	ptr->entity = it->second;

	return ptr;
}

std::shared_ptr<Item> ItemsService::duplicate(const QUuid& id) {
	auto ptr = item(id);
	if (!ptr) {
		return std::shared_ptr<Item>();
	}

	auto newPtr = std::make_shared<Item>();
	newPtr->entity = ptr->entity;
	newPtr->entityId = ptr->entityId;
	newPtr->level = ptr->level;
	newPtr->id = QUuid::createUuid();

	if (!d->itemsDataProvider->save(*newPtr)) {
		return std::shared_ptr<Item>();
	}

	return newPtr;
}

std::shared_ptr<Item> ItemsService::createItemByEntity(const QUuid& entityId) {
	auto ent = entity(entityId);
	if (!ent) {
		return std::shared_ptr<Item>();
	}

	auto newPtr = std::make_shared<Item>();
	newPtr->entity = ent;
	newPtr->entityId = ent->id;
	newPtr->level = 1;
	newPtr->id = QUuid::createUuid();

	if (!d->itemsDataProvider->save(*newPtr)) {
		return std::shared_ptr<Item>();
	}

	return newPtr;
}

std::shared_ptr<ItemEntity> ItemsService::entity(const QUuid& entityId) {
	const auto& it = d->entities.find(entityId);
	if (it == d->entities.end()) {
		return std::shared_ptr<ItemEntity>();
	}

	return it->second;
}
