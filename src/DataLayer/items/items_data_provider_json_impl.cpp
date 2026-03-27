#include "DataLayer/items/items_data_provider_json_impl.h"
#include "DataStream/format/json/data_reader.h"
#include "DataStream/format/pixmap/data_reader.h"
#include "Resources/resources.h"
#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>

class ItemsDataProviderJsonImpl::Private {
public:
	Private(ItemsDataProviderJsonImpl* paren)
		: q(paren) {
	}

	void itemEntityFromJson(const QJsonObject& json, ItemEntity& entity) {
		entity.id = json["id"].toString();
		entity.name = json["name"].toString();
		entity.description = json["description"].toString();
		entity.iconPath = json["icon"].toString();
		entity.equipmentType = ItemEquipmentType::None;

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

			if (json.contains("resource")) {
				QJsonArray resourceTypes = json["resource"].toArray();
				for (const QJsonValue& rtValue : resourceTypes) {
					const auto typeName = rtValue.toString().toLower();
					if (typeName == "ore") {
						entity.resourceType.push_back(ItemResourceType::Ore);
					}
					else if (typeName == "chemical") {
						entity.resourceType.push_back(ItemResourceType::Chemical);
					}
					else {
						qDebug() << "Load item entity: Undefined resource type:" << typeName;
					}
				}
			}
		}
		else if (typeStr == "component") {
			entity.type = ItemType::Component;
		}
		else if (typeStr == "container") {
			entity.type = ItemType::Container;

			// контейнер тоже может быть частью экипировки, но только рюкзаком или подсумком
			QString equipType = json["equipmentType"].toString().toLower();
			if (equipType == "backpack") entity.equipmentType = ItemEquipmentType::Backpack;
			else if (equipType == "bag") entity.equipmentType = ItemEquipmentType::Bag;

			QJsonObject cap = json["container"].toObject();
			entity.container = ItemContainer{
				cap["rows"].toInt(4),
				cap["cols"].toInt(4),
			};

			if (json.contains("permissions")) {
				QJsonObject permissions = json["permissions"].toObject();
				if (permissions.contains("resource")) {
					QJsonObject resource = permissions["resource"].toObject();
					if (resource.contains("all")) {
						QJsonArray allArray = resource["all"].toArray();
						for (const QJsonValue& val : allArray) {
							const auto typeName = val.toString().toLower();
							if (typeName == "ore") {
								entity.container->permissions.resources.all.push_back(ItemResourceType::Ore);
							}
							else if (typeName == "chemical") {
								entity.container->permissions.resources.all.push_back(ItemResourceType::Chemical);
							}
							else {
								qDebug() << "Load item entity: Undefined resource type in permissions.all:" << typeName;
							}
						}
					}

					if (resource.contains("any")) {
						QJsonArray anyArray = resource["any"].toArray();
						for (const QJsonValue& val : anyArray) {
							const auto typeName = val.toString().toLower();
							if (typeName == "ore") {
								entity.container->permissions.resources.any.push_back(ItemResourceType::Ore);
							}
							else if (typeName == "chemical") {
								entity.container->permissions.resources.any.push_back(ItemResourceType::Chemical);
							}
							else {
								qDebug() << "Load item entity: Undefined resource type in permissions.any:" << typeName;
							}
						}
					}
				}
			}
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

	QPixmap loadIcon(const ItemEntity& entity) {
		const auto path = QString("items/%1")
			.arg(entity.iconPath);

		QPixmap pixmap;
		Format::Pixmap::DataReader reader(resources, "assets", path);
		if (!reader.read(pixmap)) {
			return loadEmptyStubIcon(entity.id);
		}

		return pixmap;
	}

	QPixmap loadEmptyStubIcon(const QString& id) {
		QPixmap result = QPixmap(64, 64);
		result.fill(Qt::darkGray);
		QPainter painter(&result);
		painter.setPen(Qt::white);
		painter.drawText(result.rect(), Qt::AlignCenter, id.left(2));
		return result;
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

	d->itemEntityFromJson(json, entity);

	entity.icon = d->loadIcon(entity);

	return true;
}

bool ItemsDataProviderJsonImpl::loadEntitiesIds(std::list<QString>& list) const {
	const auto path = QString("items/entities_ids.json");

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

bool ItemsDataProviderJsonImpl::loadItem(const QUuid& id, Item& item) const {
	const auto uid = id
		.toString(QUuid::StringFormat::WithoutBraces)
		.toLower();
	const auto path = QString("items/%1.json")
		.arg(uid);

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "data", path);
	if (!reader.read(json)) {
		return false;
	}

	item.id = id;
	item.entityId = json["entityId"].toString();

	return true;
}
