#pragma once
#include "DataLayer/items/items_data_provider.h"
#include <QObject>
#include <map>

/// Моковый ItemsDataProvider для юнит-тестов
/// Позволяет задавать тестовые данные программно без файлов
class MockItemsDataProvider : public ItemsDataProvider {
public:
	MockItemsDataProvider() = default;
	~MockItemsDataProvider() override = default;

	// Добавить тестовую сущность предмета
	void addEntity(const QString& id, const ItemEntity& entity);
	
	// Добавить тестовый предмет
	void addItem(const QString& id, const Item& item);

	// Реализация интерфейса ItemsDataProvider
	bool loadEntitiesIds(std::list<QString>& list) const override;
	bool loadEntity(const QString& id, ItemEntity& entity) const override;
	bool loadItem(const QString& id, Item& item) const override;

private:
	std::map<QString, ItemEntity> entities;
	std::map<QString, Item> items;
};
