#include "ApplicationLayer/items/items_service.h"
#include <QString>
#include <map>

class ItemsService::Private {
public:
	Private(ItemsService* paren)
		: q(paren) {
	}

	ItemsService* q;
	std::map<QString, std::unique_ptr<Item>> items;
};

ItemsService::ItemsService()
	: d(std::make_unique<Private>(this)) {
}

ItemsService::~ItemsService() = default;


