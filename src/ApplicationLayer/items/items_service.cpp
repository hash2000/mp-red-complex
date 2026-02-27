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

	Item* itemById(const QString& id) {
		const auto& it = items.find(id);
		if (it != items.end()) {
			return it->second.get();
		}

		return nullptr;
	}

	Item* makeItem(const QString& id, ItemOwner owner) {
		auto item = itemById(id);
		if (item) {
			if (item->owner != std::nullopt && item->owner != owner) {
				qWarning() << "ItemsService::itemById: Item id " << id << "belongs to another owner";
				return nullptr;
			}

			return item;
		}

		auto newItem = std::make_unique<Item>();
		if (!dataProvider->loadItem(id, *newItem)) {
			return nullptr;
		}

		newItem->owner = owner;
		newItem->entity = entityById(newItem->entityId);

		const auto& emplaceResult = items.emplace(newItem->id, std::move(newItem));

		return emplaceResult.first->second.get();
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

const Item* ItemsService::itemById(const QString& id, ItemOwner owner) {
	return d->makeItem(id, owner);
}

const Item* ItemsService::duplicate(const QString& id, ItemOwner owner) {
	const auto from = d->itemById(id);
	if (!from) {
		return nullptr;
	}

	auto item = std::make_unique<Item>();
	item->entity = from->entity;
	item->entityId = from->entityId;
	item->owner = owner;
	item->id = QUuid::createUuid()
		.toString(QUuid::StringFormat::WithoutBraces);

	const auto& emplaceResult = d->items.emplace(item->id, std::move(item));

	return emplaceResult.first->second.get();
}

void ItemsService::changeOwner(const QString& id, std::optional<ItemOwner> owner) {
	const auto& it = d->items.find(id);
	if (it == d->items.end()) {
		return;
	}

	it->second->owner = owner;
}

