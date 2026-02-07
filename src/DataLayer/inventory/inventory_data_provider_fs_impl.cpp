#include "DataLayer/inventory/inventory_data_provider_fs_impl.h"
#include "Resources/resources.h"

class InventoryDataProviderFilesistemImpl::Private {
public:
	Private(InventoryDataProviderFilesistemImpl* parent)
	: q(parent) {
	}

	InventoryDataProviderFilesistemImpl* q;
	Resources* resources;
};


InventoryDataProviderFilesistemImpl::InventoryDataProviderFilesistemImpl(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

InventoryDataProviderFilesistemImpl::~InventoryDataProviderFilesistemImpl() = default;

QList<InventoryItem> InventoryDataProviderFilesistemImpl::fromContainer(const QUuid& id) const {
	QList<InventoryItem> result;


	return result;
}
