#include "DataLayer/items/items_data_writer_json_impl.h"
#include "DataLayer/items/item.h"
#include "Resources/resources.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>

class ItemsDataWriterJsonImpl::Private {
public:
	Private(ItemsDataWriterJsonImpl* parent)
		: q(parent) {
	}

	ItemsDataWriterJsonImpl* q;
	Resources* resources;
};

ItemsDataWriterJsonImpl::ItemsDataWriterJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

ItemsDataWriterJsonImpl::~ItemsDataWriterJsonImpl() = default;

bool ItemsDataWriterJsonImpl::saveItem(const QUuid& id, const Item& item) const {
	const auto uid = id
		.toString(QUuid::StringFormat::WithoutBraces)
		.toLower();
	const auto path = QString("data/items/%1.json")
		.arg(uid);

	QJsonObject json;
	json["id"] = uid;
	json["entityId"] = item.entityId;

	QJsonDocument doc(json);

	// Получаем путь к директории data из Resources
	auto dataDir = d->resources->Variables.get("Resources.Path", "")
		.toString();
	if (dataDir.isEmpty()) {
		qWarning() << "ItemsDataWriterJsonImpl: Resources.Path not found in variables";
		return false;
	}

	QDir dir(dataDir);
	if (!dir.exists("data/items")) {
		dir.mkpath("data/items");
	}

	QFile file(dir.filePath(path));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "ItemsDataWriterJsonImpl: can't open file for writing:" << file.fileName();
		return false;
	}

	file.write(doc.toJson());
	return true;
}
