#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/items/i_item_repository.h"
#include "ApplicationLayer/textures/images_service.h"
#include "DataLayer/images/i_images_data_provider.h"
#include <QString>
#include <QUuid>
#include <QDebug>
#include <map>

class ItemsService::Private {
public:
	Private(ItemsService* parent, std::shared_ptr<IItemRepository> repository, ImagesService* ImagesService)
		: q(parent)
		, itemRepository(repository)
		, ImagesService(ImagesService) {
	}

	const ItemEntity* entityById(const QString& id) const {
		const auto& it = itemEntities.find(id);
		if (it == itemEntities.end()) {
			return nullptr;
		}

		return it->second.get();
	}

	Item* itemById(const QUuid& id) {
		const auto& it = items.find(id);
		if (it != items.end()) {
			return it->second.get();
		}

		return nullptr;
	}

	Item* makeItem(const QUuid& id) {
		auto item = itemById(id);
		if (item) {
			return item;
		}

		// Lazy loading: загружаем предмет только когда он запрошен
		auto loadedItem = itemRepository->findItemById(id);
		if (!loadedItem) {
			qWarning() << "ItemsService::makeItem: failed to load item" << id;
			return nullptr;
		}

		// Находим сущность для предмета (тоже с lazy loading)
		auto entityIt = itemEntities.find(loadedItem->entityId);
		if (entityIt == itemEntities.end()) {
			auto loadedEntity = itemRepository->findEntityById(loadedItem->entityId);
			if (!loadedEntity) {
				qWarning() << "ItemsService::makeItem: unknown item entity" << loadedItem->entityId;
				return nullptr;
			}
			loadedItem->entity = loadedEntity.get();
			itemEntities.emplace(loadedItem->entityId, std::move(loadedEntity));
			entityIt = itemEntities.find(loadedItem->entityId);
		} else {
			loadedItem->entity = entityIt->second.get();
		}

		const auto& emplaceResult = items.emplace(id, std::move(loadedItem));

		return emplaceResult.first->second.get();
	}

	Item* createItemByEntity(const QString& entityId) {
		auto entity = entityById(entityId);
		if (!entity) {
			// Lazy loading сущности
			auto loadedEntity = itemRepository->findEntityById(entityId);
			if (!loadedEntity) {
				qWarning() << "ItemsService::createItemByEntity: unknown item entity" << entityId;
				return nullptr;
			}
			entity = loadedEntity.get();
			itemEntities.emplace(entityId, std::move(loadedEntity));
		}

		auto newItem = std::make_unique<Item>();
		newItem->entity = entity;
		newItem->entityId = entityId;
		newItem->id = QUuid::createUuid();

		const auto& emplaceResult = items.emplace(newItem->id, std::move(newItem));

		return emplaceResult.first->second.get();
	}

	ItemsService* q;
	std::shared_ptr<IItemRepository> itemRepository;
	ImagesService* ImagesService = nullptr;
	std::map<QString, std::unique_ptr<ItemEntity>> itemEntities;
	std::map<QUuid, std::unique_ptr<Item>> items;
};

ItemsService::ItemsService(
	std::shared_ptr<IItemRepository> repository,
	ImagesService* ImagesService,
	QObject* parent)
	: d(std::make_unique<Private>(this, repository, ImagesService))
	, QObject(parent) {
}

ItemsService::~ItemsService() = default;

void ItemsService::loadEntities() {
	// Загружаем все идентификаторы сущностей
	const auto entityIds = d->itemRepository->findAllEntityIds();

	// Загружаем каждую сущность заранее (eager loading)
	for (const auto& entityId : entityIds) {
		auto entity = d->itemRepository->findEntityById(entityId);
		if (entity) {
			// Загружаем иконку сущности через ImagesService
			if (!entity->iconPath.isEmpty() && d->ImagesService) {
				entity->icon = d->ImagesService->getImage(entity->iconPath, ImageType::Item);
			}

			d->itemEntities.emplace(entityId, std::move(entity));
		}
		else {
			qWarning() << "ItemsService::loadEntities: failed to load entity" << entityId;
		}
	}

	qInfo() << "ItemsService::loadEntities: loaded" << d->itemEntities.size() << "item entities";
}

ItemsService::EntityView ItemsService::entities() const {
	return make_deref_view(d->itemEntities);
}

const ItemEntity* ItemsService::entityById(const QString& entityId) const {
	return d->entityById(entityId);
}

const Item* ItemsService::itemById(const QUuid& id) {
	return d->makeItem(id);
}

ItemsService::ItemView ItemsService::items() const {
	return make_deref_view(d->items);
}

const Item* ItemsService::duplicate(const QUuid& id) {
	const auto from = d->itemById(id);
	if (!from) {
		return nullptr;
	}

	auto item = std::make_unique<Item>();
	item->entity = from->entity;
	item->entityId = from->entityId;
	item->id = QUuid::createUuid();

	const auto& emplaceResult = d->items.emplace(item->id, std::move(item));

	return emplaceResult.first->second.get();
}

const Item* ItemsService::createItemByEntity(const QString& entityId) {
	return d->createItemByEntity(entityId);
}
