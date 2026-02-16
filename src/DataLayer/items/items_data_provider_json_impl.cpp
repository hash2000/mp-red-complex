#include "DataLayer/items/items_data_provider_json_impl.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"
#include <QPainter>
#include <QJsonObject>

class ItemsDataProviderJsonImpl::Private {
public:
	Private(ItemsDataProviderJsonImpl* paren)
		: q(paren) {
	}

	ItemsDataProviderJsonImpl* q;
	Resources* resources;
};

ItemsDataProviderJsonImpl::ItemsDataProviderJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

ItemsDataProviderJsonImpl::~ItemsDataProviderJsonImpl() = default;

bool ItemsDataProviderJsonImpl::load(const QString& id, Item& item) const {
	const auto path = QString("items/%1.json")
		.arg(item.entityId);

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "assets", path);
	if (!reader.read(json)) {
		return false;
	}

	//item.icon = loadIcon(item);
	return true;
}

QPixmap ItemsDataProviderJsonImpl::loadIcon(const Item& item) const {
	QPixmap result;
	const auto path = QString("items/%1")
		.arg(item.iconPath);

	//auto stream = d->resources->getStream("assets", path);
	//if (!stream) {
	//	return loadEmptyStubIcon(item.id);
	//}

	//try {
	//	auto block = stream.value()->makeBlockAsStream();
	//	DataFormat::Pixmap::DataReader reader(*block);
	//	reader.read(result);
	//}
	//catch (std::exception& ex) {
	//	qDebug() << ex.what();
	//	return loadEmptyStubIcon(item.id);
	//}

	return result;
}

QPixmap ItemsDataProviderJsonImpl::loadEmptyStubIcon(const QString& id) {
	QPixmap result = QPixmap(64, 64);
	result.fill(Qt::darkGray);
	QPainter painter(&result);
	painter.setPen(Qt::white);
	painter.drawText(result.rect(), Qt::AlignCenter, id.left(2));
	return result;
}
