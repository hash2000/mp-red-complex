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
	QJsonObject json;
	json["id"] = inventory.id
		.toString(QUuid::StringFormat::WithoutBraces)
		.toLower();
	json["name"] = inventory.name;
	json["rows"] = inventory.rows;
	json["cols"] = inventory.cols;

	QJsonArray itemsArr;
	for (const auto& item : inventory.items) {
		QJsonObject itemObj;
		itemObj["id"] = item.id
			.toString(QUuid::StringFormat::WithoutBraces)
			.toLower();
		itemObj["count"] = item.count;
		itemObj["x"] = item.x;
		itemObj["y"] = item.y;
		itemsArr.append(itemObj);
	}
	json["items"] = itemsArr;

	QJsonDocument doc(json);

	// Получаем путь к директории data из Resources
	const auto path = QString("data/inventory/");
	auto dataDir = d->resources->Variables.get("Resources.Path", "")
		.toString();
	if (dataDir.isEmpty()) {
		qWarning() << "InventoryDataWriterJsonImpl: Resources.Path not found in variables";
		return false;
	}

	QDir dir(dataDir);
	if (!dir.exists(path)) {
		dir.mkpath(path);
	}

	const auto fileName = id.toString(QUuid::StringFormat::WithoutBraces).toLower() + ".json";
	QFile file(dir.filePath(path + fileName));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "InventoryDataWriterJsonImpl: can't open file for writing:" << file.fileName();
		return false;
	}

	file.write(doc.toJson());
	return true;
}
