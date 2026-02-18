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

	using ItemsView = decltype(make_deref_view(std::declval<const std::map<QString, std::unique_ptr<ItemEntity>>&>()));
	ItemsView items() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
