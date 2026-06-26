#include "Content/InventoriesModule/data_providers/readers/entity_reader.h"
#include "Content/InventoriesModule/models/item.h"
#include "Content/InventoriesModule/models/extensions/entity_extensions.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"
#include <QDebug>

namespace Inventory::Readers {
EntityReader::EntityReader(std::shared_ptr<ItemEntity> entry)
	: _entry(entry) {
	if (!_entry) {
		_entry = std::make_shared<ItemEntity>();
	}
}

std::shared_ptr<ItemEntity> EntityReader::read(SQLiteReader& reader) {
	using namespace Inventory::Entities;

	_entry->id = reader.value("id").toString();
	_entry->name = reader.value("name").toString();
	_entry->type = itemType(reader.value("type").toString());
	_entry->subType = itemSubType(reader.value("sub_type").toString());
	_entry->description = reader.value("description").toString();
	_entry->iconPath = reader.value("icon_path").toString();
	_entry->width = reader.value("width").toInt();
	_entry->height = reader.value("height").toInt();
	_entry->rarity = static_cast<ItemRarityType>(reader.value("rarity").toInt());
	_entry->maxStack = reader.value("max_stack").toInt();

	if (_entry->type == ItemType::Resource) {
		const QString resourceIds = reader.value("resource_ids").toString();
		const QStringList ids = resourceIds.split(",", Qt::SkipEmptyParts);

		for (const auto& typeName : ids) {
			_entry->resourceType.push_back(resourceType(typeName));
		}
	}
	else if (_entry->type == ItemType::Container) {
		const QString permissions_for_all = reader.value("permissions_for_all").toString();
		const QStringList all = permissions_for_all.split(",", Qt::SkipEmptyParts);
		const QString permissions_for_any = reader.value("permissions_for_any").toString();
		const QStringList any = permissions_for_any.split(",", Qt::SkipEmptyParts);

		_entry->container = ItemContainer();
		_entry->container->rows = reader.value("container_rows").toInt();
		_entry->container->cols = reader.value("container_cols").toInt();

		for (const auto& typeName : all) {
			_entry->container->permissions.resources.all.push_back(resourceType(typeName));
		}

		for (const auto& typeName : any) {
			_entry->container->permissions.resources.any.push_back(resourceType(typeName));
		}
	}

	return _entry;
}
}
