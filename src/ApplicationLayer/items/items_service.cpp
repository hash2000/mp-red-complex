#include "ApplicationLayer/items/items_service.h"
#include "DataLayer/items/items_data_provider.h"
#include <QString>
#include <map>

class ItemsService::Private {
public:
	Private(ItemsService* paren)
		: q(paren) {
	}

	ItemsService* q;
	ItemsDataProvider* dataProvider;
	std::map<QString, std::unique_ptr<ItemEntity>> itemEntities;
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

	//qDebug() << "Loading" << count << "items entities";
	return true;
}

ItemsService::ItemsView ItemsService::items() const {
	return make_deref_view(d->itemEntities);
}
