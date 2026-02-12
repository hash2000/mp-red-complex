#include "DataLayer/inventory/inventory_data_provider_fs_impl.h"
#include "Resources/resources.h"
#include "DataFormat/data_format/json/data_reader.h"
#include "DataFormat/data_format/pixmap/data_reader.h"
#include <QPainter>

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

std::shared_ptr<Inventory> InventoryDataProviderFilesistemImpl::loadInventory(const QUuid& id) const {
	const auto path = QString("inventory/%1.json")
		.arg(id.toString().toLower());

	auto stream = d->resources->getStream("data", path);
	if (!stream) {
		return std::shared_ptr<Inventory>();
	}

	QJsonObject json;
	try {
		auto block = stream.value()->makeBlockAsStream();
		DataFormat::Json::DataReader reader(*block);
		reader.read(json);
	}
	catch (std::exception& ex) {
		qDebug() << ex.what();
		return std::shared_ptr<Inventory>();
	}

	auto result = std::make_shared<Inventory>();
	result->id = id.toString(QUuid::StringFormat::WithoutBraces);
	result->rows = json["rows"].toInt();
	result->cols = json["cols"].toInt();

	QJsonArray items = json["items"].toArray();
	for (const QJsonValue& it : items) {
		const auto entityId = it["entityId"]
			.toString()
			.toLower();
		const auto id = it["id"]
			.toString()
			.toLower();

		if (id.isEmpty()) {
			qDebug() << "Inventory" << path << "has item without id field. entityId =" << entityId;
			continue;
		}

		const auto x = it["x"].toInt();
		const auto y = it["y"].toInt();
		auto invItem = loadItem(entityId);
		if (!invItem) {
			continue;
		}

		invItem->id = id;
		invItem->x = x;
		invItem->y = y;
		result->items.emplace(invItem->id, invItem);
	}

	return result;
}

std::shared_ptr<InventoryItem> InventoryDataProviderFilesistemImpl::loadItem(const QString& id) const {
	const auto path = QString("items/%1.json")
		.arg(id);

	auto stream = d->resources->getStream("assets", path);
	if (!stream) {
		qDebug().nospace() << "Can't open stream [" << "assets" << "] " << path;
		return std::shared_ptr<InventoryItem>();
	}

	std::shared_ptr<InventoryItem> result;
	QJsonObject json;
	try {
		auto block = stream.value()->makeBlockAsStream();
		DataFormat::Json::DataReader reader(*block);
		reader.read(json);
	}
	catch (std::exception& ex) {
		qDebug() << ex.what();
		return std::shared_ptr<InventoryItem>();
	}

	result = InventoryItem::fromJson(json);
	result->icon = loadIcon(*result);
	return result;
}

QPixmap InventoryDataProviderFilesistemImpl::loadIcon(const InventoryItem& item) const {
	QPixmap result;
	const auto path = QString("items/%1")
		.arg(item.iconPath);

	auto stream = d->resources->getStream("assets", path);
	if (!stream) {
		return loadEmptyStubIcon(item.id);
	}

	try {
		auto block = stream.value()->makeBlockAsStream();
		DataFormat::Pixmap::DataReader reader(*block);
		reader.read(result);
	}
	catch (std::exception& ex) {
		qDebug() << ex.what();
		return loadEmptyStubIcon(item.id);
	}

	return result;
}

QPixmap InventoryDataProviderFilesistemImpl::loadEmptyStubIcon(const QString& id) {
	QPixmap result = QPixmap(64, 64);
	result.fill(Qt::darkGray);
	QPainter painter(&result);
	painter.setPen(Qt::white);
	painter.drawText(result.rect(), Qt::AlignCenter, id.left(2));
	return result;
}
