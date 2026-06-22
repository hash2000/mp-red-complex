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
	std::map<QString, std::shared_ptr<ItemEntity>> entities;

	void loadEntitieas();
	bool populateItem(std::shared_ptr<Item> item);
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
		const auto iconPath = i->iconPath;
		if (!iconPath.isEmpty()) {
			i->icon = imagesService->getImage(iconPath, ImageType::Item);
		}

		entities.emplace(i->id, i);
	}
}

bool ItemsService::Private::populateItem(std::shared_ptr<Item> item) {
	if (item->entityId.isEmpty()) {
		qWarning() << "Load item" << item->id << ". Undefined entity";
		return false;
	}

	const auto& it = entities.find(item->entityId);
	if (it == entities.end()) {
		qWarning() << "Load item" << item->id << ". Entity is unknown";
		return false;
	}

	item->entity = it->second;
	return true;
}

std::shared_ptr<Item> ItemsService::item(const QUuid& id) {
	auto ptr = d->itemsDataProvider->item(id);
	if (!ptr) {
		qWarning() << "Load item" << id << ". Undefined item";
		return std::shared_ptr<Item>();
	}

	if (!d->populateItem(ptr)) {
		return std::shared_ptr<Item>();
	}

	return ptr;
}

std::list<std::shared_ptr<Item>> ItemsService::itemsFromContainer(const QUuid& containerId) {
	auto items = d->itemsDataProvider->itemsFromContainer(containerId);

	for (auto item : items) {
		if (!d->populateItem(item)) {
			return std::list<std::shared_ptr<Item>>();
		}
	}

	return items;
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

std::shared_ptr<Item> ItemsService::createItemByEntity(const QString& entityId) {
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

std::shared_ptr<ItemEntity> ItemsService::entity(const QString& entityId) {
	const auto& it = d->entities.find(entityId);
	if (it == d->entities.end()) {
		return std::shared_ptr<ItemEntity>();
	}

	return it->second;
}
