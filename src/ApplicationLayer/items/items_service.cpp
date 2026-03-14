#include "ApplicationLayer/items/items_service.h"
#include "DataLayer/items/items_data_provider.h"
#include <QString>
#include <QUuid>
#include <QDebug>
#include <map>

class ItemsService::Private {
public:
	Private(ItemsService* paren)
		: q(paren) {
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

		auto newItem = std::make_unique<Item>();
		if (!dataProvider->loadItem(id, *newItem)) {
			return nullptr;
		}

		newItem->entity = entityById(newItem->entityId);
		if (!newItem->entity) {
			qWarning() << "Unknown item entity" << newItem->entityId;
			return nullptr;
		}

		const auto& emplaceResult = items.emplace(newItem->id, std::move(newItem));

		return emplaceResult.first->second.get();
	}

	Item* createItemByEntity(const QString& entityId) {
		auto newItem = std::make_unique<Item>();
		newItem->entity = entityById(entityId);
		if (!newItem->entity) {
			qWarning() << "Unknown item entity" << entityId;
			return nullptr;
		}

		newItem->id = QUuid::createUuid();
		newItem->entityId = entityId;

		const auto& emplaceResult = items.emplace(newItem->id, std::move(newItem));

		return emplaceResult.first->second.get();

	}

	ItemsService* q;
	ItemsDataProvider* dataProvider;
	std::map<QString, std::unique_ptr<ItemEntity>> itemEntities;
	std::map<QUuid, std::unique_ptr<Item>> items;
};

ItemsService::ItemsService(ItemsDataProvider* dataProvider, QObject* parent)
	: d(std::make_unique<Private>(this))
	, QObject(parent) {
	d->dataProvider = dataProvider;
}

ItemsService::~ItemsService() = default;

bool ItemsService::loadEntities() {
	std::list<QString> list;
	if (!d->dataProvider->loadEntitiesIds(list)) {
		return false;
	}

	int count = 0;
	for (const auto& id : list) {
		auto entity = std::make_unique<ItemEntity>();
		if (!d->dataProvider->loadEntity(id, *entity)) {
			continue;
		}

		count++;
		d->itemEntities.emplace(id, std::move(entity));
	}

	qDebug() << "Loading" << count << "items entities";
	return true;
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


