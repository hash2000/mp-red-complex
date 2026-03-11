#include "mock_items_data_provider.h"

void MockItemsDataProvider::addEntity(const QString& id, const ItemEntity& entity) {
	entities[id] = entity;
}

void MockItemsDataProvider::addItem(const QString& id, const Item& item) {
	items[id] = item;
}

bool MockItemsDataProvider::loadEntitiesIds(std::list<QString>& list) const {
	for (const auto& [id, entity] : entities) {
		list.push_back(id);
	}
	return !entities.empty();
}

bool MockItemsDataProvider::loadEntity(const QString& id, ItemEntity& entity) const {
	const auto it = entities.find(id);
	if (it == entities.end()) {
		return false;
	}
	entity = it->second;
	return true;
}

bool MockItemsDataProvider::loadItem(const QString& id, Item& item) const {
	const auto it = items.find(id);
	if (it == items.end()) {
		return false;
	}
	item = it->second;
	return true;
}
