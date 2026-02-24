#include "DataLayer/inventory/inventory_data_provider_json_impl.h"
#include "DataLayer/inventory/inventory_item.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"
#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>

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

bool InventoryDataProviderJsonImpl::loadInventory(const QUuid& id, Inventory& inventory) const {
	const auto path = QString("inventory/%1.json")
		.arg(id.toString(QUuid::StringFormat::WithoutBraces).toLower());

	auto stream = d->resources->getStream("data", path);
	if (!stream) {
		return false;
	}

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "data", path);
	if (!reader.read(json)) {
		return false;
	}

	inventory.id = id
		.toString(QUuid::StringFormat::WithoutBraces)
		.toLower();
	inventory.rows = json["rows"].toInt();
	inventory.cols = json["cols"].toInt();
	inventory.name = json["name"].toString();
	const auto itemsArr = json["items"].toArray();

	for (const QJsonValue& itemObj : itemsArr) {
		InventoryItem item;
		item.id = itemObj["id"].toString();
		item.count = itemObj["count"].toInt();
		item.x = itemObj["x"].toInt();
		item.y = itemObj["y"].toInt();
		inventory.items.push_back(item);
	}

	return true;
}
