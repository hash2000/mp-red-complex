#include "DataLayer/inventory/inventory_data_provider_json_impl.h"
#include "Resources/resources.h"
#include "DataFormat/data_format/item/data_reader_json.h"
#include "DataFormat/data_format/inventory/data_reader_json.h"
#include "DataFormat/data_format/pixmap/data_reader.h"
#include "DataFormat/proto/inventory_item.h"

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

std::shared_ptr<Inventory> InventoryDataProviderJsonImpl::loadInventory(const QUuid& id) const {
	const auto path = QString("inventory/%1.json")
		.arg(id.toString().toLower());

	auto stream = d->resources->getStream("data", path);
	if (!stream) {
		return std::shared_ptr<Inventory>();
	}

	auto result = std::make_shared<Inventory>();

	try {
		auto block = stream.value()->makeBlockAsStream();
		DataFormat::Inv::DataReaderJson reader(*block);
		reader.read(*result);
	}
	catch (std::exception& ex) {
		qDebug() << ex.what();
		return std::shared_ptr<Inventory>();
	}

	result->id = id.toString(QUuid::StringFormat::WithoutBraces);

	for (const auto& it : result->items) {
		loadItem(*it.second);		
	}

	return result;
}

bool InventoryDataProviderJsonImpl::loadItem(InventoryItem& item) const {
	const auto path = QString("items/%1.json")
		.arg(item.entityId);

	auto stream = d->resources->getStream("assets", path);
	if (!stream) {
		qDebug().nospace() << "Can't open stream [" << "assets" << "] " << path;
		return false;
	}

	try {
		auto block = stream.value()->makeBlockAsStream();
		DataFormat::Item::DataReaderJson reader(*block);
		reader.read(item);
	}
	catch (std::exception& ex) {
		qDebug() << ex.what();
		return false;
	}

	item.icon = loadIcon(item);
	return true;
}

QPixmap InventoryDataProviderJsonImpl::loadIcon(const InventoryItem& item) const {
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

QPixmap InventoryDataProviderJsonImpl::loadEmptyStubIcon(const QString& id) {
	QPixmap result = QPixmap(64, 64);
	result.fill(Qt::darkGray);
	QPainter painter(&result);
	painter.setPen(Qt::white);
	painter.drawText(result.rect(), Qt::AlignCenter, id.left(2));
	return result;
}
