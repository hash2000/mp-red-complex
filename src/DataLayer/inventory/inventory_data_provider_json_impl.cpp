#include "DataLayer/inventory/inventory_data_provider_json_impl.h"
#include "Resources/resources.h"
#include <QPainter>

class InventoryDataProviderJsonImpl::Private {
public:
	Private(InventoryDataProviderJsonImpl* parent)
	: q(parent) {
	}

	InventoryDataProviderJsonImpl* q;
	Resources* resources;
};


InventoryDataProviderJsonImpl::InventoryDataProviderJsonImpl(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

InventoryDataProviderJsonImpl::~InventoryDataProviderJsonImpl() = default;

bool InventoryDataProviderJsonImpl::loadInventory(const QUuid& id, InventoryRaw& inventory) const {
	const auto path = QString("inventory/%1.json")
		.arg(id.toString(QUuid::StringFormat::WithoutBraces).toLower());

	auto stream = d->resources->getStream("data", path);
	if (!stream) {
		return false;
	}



	return true;
}
