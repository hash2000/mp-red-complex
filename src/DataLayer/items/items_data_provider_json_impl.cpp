#include "DataLayer/items/items_data_provider_json_impl.h"
#include "DataStream/format/json/data_reader.h"
#include "DataStream/format/pixmap/data_reader.h"
#include "Resources/resources.h"
#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>

namespace {
	void itemEntityFromJson(const QJsonObject& json, ItemEntity& entity) {
		entity.id = json["id"].toString();
		entity.name = json["name"].toString();
		entity.description = json["description"].toString();
		entity.iconPath = json["icon"].toString();

		// Тип предмета
		QString typeStr = json["type"].toString().toLower();
		if (typeStr == "equipment") {
			entity.type = ItemType::Equipment;
			QString equipType = json["equipmentType"].toString().toLower();
			if (equipType == "head") entity.equipmentType = ItemEquipmentType::Head;
			else if (equipType == "body") entity.equipmentType = ItemEquipmentType::Body;
			else if (equipType == "weapon") entity.equipmentType = ItemEquipmentType::Weapon;
			else if (equipType == "shield") entity.equipmentType = ItemEquipmentType::Shield;
			else if (equipType == "gloves") entity.equipmentType = ItemEquipmentType::Gloves;
			else if (equipType == "boots") entity.equipmentType = ItemEquipmentType::Boots;
			else if (equipType == "ring") entity.equipmentType = ItemEquipmentType::Ring;
			else if (equipType == "amulet") entity.equipmentType = ItemEquipmentType::Amulet;
		}
		else if (typeStr == "resource") {
			entity.type = ItemType::Resource;
			entity.maxStack = json["maxStack"].toInt(100);
		}
		else if (typeStr == "component") {
			entity.type = ItemType::Component;
			entity.maxStack = json["maxStack"].toInt(100);
		}
		else if (typeStr == "container") {
			entity.type = ItemType::Container;
		}

		// Размер в ячейках
		entity.width = json["width"].toInt(1);
		entity.height = json["height"].toInt(1);

		// Рецепт крафта (опционально)
		if (json.contains("recipe")) {
			entity.recipe = ItemRecipe();
			QJsonArray ingredients = json["recipe"]
				.toObject()["ingredients"]
				.toArray();

			for (const QJsonValue& ingVal : ingredients) {
				QJsonObject ingObj = ingVal.toObject();
				entity.recipe->ingredients.push_back({
						ingObj["itemId"].toString(),
						ingObj["amount"].toInt()
					});
			}
		}
	}
}



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

bool ItemsDataProviderJsonImpl::loadEntity(const QString& id, ItemEntity& entity) const {
	const auto path = QString("items/%1.json")
		.arg(id);

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "assets", path);
	if (!reader.read(json)) {
		return false;
	}

	itemEntityFromJson(json, entity);

	entity.icon = loadIcon(entity);

	return true;
}

QPixmap ItemsDataProviderJsonImpl::loadIcon(const ItemEntity& entity) const {
	const auto path = QString("items/%1")
		.arg(entity.iconPath);

	QPixmap pixmap;
	Format::Pixmap::DataReader reader(d->resources, "assets", path);
	if (!reader.read(pixmap)) {
		return loadEmptyStubIcon(entity.id);
	}

	return pixmap;
}

QPixmap ItemsDataProviderJsonImpl::loadEmptyStubIcon(const QString& id) {
	QPixmap result = QPixmap(64, 64);
	result.fill(Qt::darkGray);
	QPainter painter(&result);
	painter.setPen(Qt::white);
	painter.drawText(result.rect(), Qt::AlignCenter, id.left(2));
	return result;
}

bool ItemsDataProviderJsonImpl::loadEntitiesIds(std::list<QString>& list) {
	const auto path = QString("items/items_ids.json");

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "assets", path);
	if (!reader.read(json)) {
		return false;
	}

	const QJsonArray items = json["items"]
		.toArray();

	for (const auto& item : items) {
		list.push_back(item.toString());
	}

	return true;
}
