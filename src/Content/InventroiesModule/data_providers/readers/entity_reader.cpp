#include "Content/InventroiesModule/data_providers/readers/entity_reader.h"
#include "Content/InventroiesModule/models/item.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"
#include <QDebug>

namespace DataProviders::Readers {
EntityReader::EntityReader(std::shared_ptr<ItemEntity> entity)
	: _entity(entity) {
	if (!_entity) {
		_entity = std::make_shared<ItemEntity>();
	}
}

std::shared_ptr<ItemEntity> EntityReader::read(SQLiteReader& reader) {
	_entity->id = reader.value("id").toString();
	_entity->name = reader.value("name").toString();
	_entity->type = itemType(reader.value("type").toString());
	_entity->subType = itemSubType(reader.value("sub_type").toString());
	_entity->description = reader.value("description").toString();
	_entity->iconPath = reader.value("icon_path").toString();
	_entity->width = reader.value("width").toInt();
	_entity->height = reader.value("height").toInt();
	_entity->rarity = static_cast<ItemRarityType>(reader.value("rarity").toInt());
	_entity->maxStack = reader.value("max_stack").toInt();

	if (_entity->type == ItemType::Resource) {
		const QString resourceIds = reader.value("resource_ids").toString();
		const QStringList ids = resourceIds.split(",", Qt::SkipEmptyParts);

		for (const auto& typeName : ids) {
			_entity->resourceType.push_back(resourceType(typeName));
		}
	}
	else if (_entity->type == ItemType::Container) {
		const QString permissions_for_all = reader.value("permissions_for_all").toString();
		const QStringList all = permissions_for_all.split(",", Qt::SkipEmptyParts);
		const QString permissions_for_any = reader.value("permissions_for_any").toString();
		const QStringList any = permissions_for_any.split(",", Qt::SkipEmptyParts);

		_entity->container = ItemContainer();
		_entity->container->rows = reader.value("container_rows").toInt();
		_entity->container->cols = reader.value("container_cols").toInt();

		for (const auto& typeName : all) {
			_entity->container->permissions.resources.all.push_back(resourceType(typeName));
		}

		for (const auto& typeName : any) {
			_entity->container->permissions.resources.any.push_back(resourceType(typeName));
		}
	}

	return _entity;
}

ItemResourceType EntityReader::resourceType(const QString& type) {
	if (type == "ore") return ItemResourceType::Ore;
	else if (type == "chemical") return ItemResourceType::Chemical;
	else
		qWarning() << "Load item entity : Undefined resource type:" << type;
	return ItemResourceType::Undefined;
}

ItemType EntityReader::itemType(const QString& type) {
	if (type == "resource") return ItemType::Resource;
	else if (type == "equipment") return ItemType::Equipment;
	else if (type == "component") return ItemType::Component;
	else if (type == "container") return ItemType::Container;
	else if (type == "recipe") return ItemType::Recipe;
	else
		qWarning() << "Load item entity: Undefined type:" << type;
	return ItemType::Undefined;
}

ItemSubType EntityReader::itemSubType(const QString& type) {
	if (type == "head") return ItemSubType::Head;
	else if (type == "body") return ItemSubType::Body;
	else if (type == "weapon") return ItemSubType::Weapon;
	else if (type == "shield") return ItemSubType::Shield;
	else if (type == "gloves") return ItemSubType::Gloves;
	else if (type == "boots") return ItemSubType::Boots;
	else if (type == "ring") return ItemSubType::Ring;
	else if (type == "amulet") return ItemSubType::Amulet;
	else if (type == "resource") return ItemSubType::Resource;
	else if (type == "consumable") return ItemSubType::Consumable;
	else if (type == "backpack") return ItemSubType::Backpack;
	else if (type == "bag") return ItemSubType::Bag;
	else if (type == "root") return ItemSubType::Root;
	else
		qWarning() << "Load item entity: Undefined sub type:" << type;
	return ItemSubType::Undefined;
}

}
