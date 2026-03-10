#include "DataLayer/inventory/inventory_data_writer_json_impl.h"
#include "DataLayer/inventory/inventory_item.h"
#include "Resources/resources.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

class InventoryDataWriterJsonImpl::Private {
public:
	Private(InventoryDataWriterJsonImpl* parent)
		: q(parent) {
	}

	InventoryDataWriterJsonImpl* q;
	Resources* resources;
};

InventoryDataWriterJsonImpl::InventoryDataWriterJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

InventoryDataWriterJsonImpl::~InventoryDataWriterJsonImpl() = default;

bool InventoryDataWriterJsonImpl::saveInventory(const QUuid& id, const Inventory& inventory) const {
	const auto path = QString("data/inventory/%1.json")
		.arg(id.toString(QUuid::StringFormat::WithoutBraces).toLower());

	QJsonObject json;
	json["id"] = inventory.id;
	json["name"] = inventory.name;
	json["rows"] = inventory.rows;
	json["cols"] = inventory.cols;

	QJsonArray itemsArr;
	for (const auto& item : inventory.items) {
		QJsonObject itemObj;
		itemObj["id"] = item.id;
		itemObj["count"] = item.count;
		itemObj["x"] = item.x;
		itemObj["y"] = item.y;
		itemsArr.append(itemObj);
	}
	json["items"] = itemsArr;

	QJsonDocument doc(json);

	// Получаем путь к директории data из Resources
	auto dataDir = d->resources->Variables.get("data_dir");
	if (dataDir.isEmpty()) {
		qWarning() << "InventoryDataWriterJsonImpl: data_dir not found in variables";
		return false;
	}

	QDir dir(dataDir);
	if (!dir.exists("inventory")) {
		dir.mkpath("inventory");
	}

	QFile file(dir.filePath("inventory/" + id.toString(QUuid::StringFormat::WithoutBraces).toLower() + ".json"));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "InventoryDataWriterJsonImpl: can't open file for writing:" << file.fileName();
		return false;
	}

	file.write(doc.toJson());
	return true;
}
