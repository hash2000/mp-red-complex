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

std::optional<Inventory> InventoryDataProviderFilesistemImpl::fromContainer(const QUuid& id) const {
	const auto path = QString("inventory/%1.json")
		.arg(id.toString().toLower());

	auto stream = d->resources->getStream("data", path);
	if (!stream) {
		return std::nullopt;
	}

	QJsonObject json;
	try {
		auto block = stream.value()->makeBlockAsStream();
		DataFormat::Json::DataReader reader(*block);
		reader.read(json);
	}
	catch (std::exception& ex) {
		qDebug() << ex.what();
		return std::nullopt;
	}

	Inventory result;
	result.rows = json["rows"].toInt();
	result.cols = json["cols"].toInt();

	QJsonArray items = json["items"].toArray();
	for (const QJsonValue& item : items) {
		const auto id = item["id"].toString();
		const auto x = item["x"].toInt();
		const auto y = item["y"].toInt();
		auto invItemOpt = getItem(id);
		if (!invItemOpt) {
			continue;
		}

		auto invItem = invItemOpt.value();
		invItem.x = x;
		invItem.y = y;
		result.items.append(invItem);
	}

	return result;
}

std::optional<InventoryItem> InventoryDataProviderFilesistemImpl::getItem(const QString& id) const {
	const auto path = QString("items/%1.json")
		.arg(id);

	auto stream = d->resources->getStream("assets", path);
	if (!stream) {
		qDebug().nospace() << "Can't open stream [" << "assets" << "] " << path;
		return std::nullopt;
	}

	InventoryItem result;
	QJsonObject json;
	try {
		auto block = stream.value()->makeBlockAsStream();
		DataFormat::Json::DataReader reader(*block);
		reader.read(json);
	}
	catch (std::exception& ex) {
		qDebug() << ex.what();
		return std::nullopt;
	}

	result = InventoryItem::fromJson(json);
	result.icon = loadIcon(result);
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
