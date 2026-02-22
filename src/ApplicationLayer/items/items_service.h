#pragma once
#include "DataLayer/items/item.h"
#include "Base/container_view.h"
#include <QObject>
#include <memory>
#include <ranges>

struct ItemsDataProvider;

class ItemsService : public QObject {
	Q_OBJECT
public:
	ItemsService(ItemsDataProvider* dataProvider, QObject* parent = nullptr);
	~ItemsService() override;

	bool loadEntities();

	using EntityView = decltype(make_deref_view(std::declval<const std::map<QString, std::unique_ptr<ItemEntity>>&>()));
	EntityView entities() const;

	const ItemEntity* entityById(const QString& id) const;
	const Item* itemById(const QString& id);
	const Item* createItem(const QString& entittyId);
	const Item* duplicate(const QString& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
