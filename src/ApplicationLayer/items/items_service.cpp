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

	ItemsService* q;
	ItemsDataProvider* dataProvider;
	std::map<QString, std::unique_ptr<ItemEntity>> itemEntities;
	std::map<QString, std::unique_ptr<Item>> items;
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

const ItemEntity* ItemsService::entityById(const QString& id) const {
	const auto& it = d->itemEntities.find(id);
	if (it == d->itemEntities.end()) {
		return nullptr;
	}

	return it->second.get();
}

const Item* ItemsService::itemById(const QString& id) {
	const auto& it = d->items.find(id);
	if (it != d->items.end()) {
		return it->second.get();
	}

	auto item = std::make_unique<Item>();
	if (!d->dataProvider->loadItem(id, *item)) {
		return nullptr;
	}

	item->entity = entityById(item->entityId);

	const auto& emplaceResult = d->items.emplace(item->id, std::move(item));

	return emplaceResult.first->second.get();
}

const Item* ItemsService::createItem(const QString& entittyId) {
	const auto entity = entityById(entittyId);
	if (!entity) {
		return nullptr;
	}

	auto item = std::make_unique<Item>();
	item->entityId = entittyId;
	item->entity = entity;
	item->id = QUuid::createUuid()
		.toString(QUuid::StringFormat::WithoutBraces)
		.toLower();

	const auto& emplaceResult = d->items.emplace(item->id, std::move(item));

	return emplaceResult.first->second.get();
}

const Item* ItemsService::duplicate(const QString& id) {
	const auto from = itemById(id);
	if (!from) {
		return nullptr;
	}

	auto item = std::make_unique<Item>();
	item->entity = from->entity;
	item->entityId = from->entityId;
	item->id = QUuid::createUuid()
		.toString(QUuid::StringFormat::WithoutBraces);

	const auto& emplaceResult = d->items.emplace(item->id, std::move(item));

	return emplaceResult.first->second.get();
}
