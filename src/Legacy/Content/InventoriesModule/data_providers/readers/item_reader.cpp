#include "Content/InventoriesModule/data_providers/readers/item_reader.h"
#include "Content/InventoriesModule/models/item.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"


namespace Inventory::Readers {
ItemReader::ItemReader(std::shared_ptr<Item> entry)
	: _entry(entry) {
	if (!_entry) {
		_entry = std::make_shared<Item>();
	}
}

std::shared_ptr<Item> ItemReader::read(SQLiteReader& reader) {
	_entry->id = QUuid::fromString(reader.value("id").toString());
	_entry->entityId = reader.value("entity_id").toString();
	_entry->containerId = QUuid::fromString(reader.value("container_id").toString());
	_entry->level = reader.value("level").toInt();
	_entry->count = reader.value("count").toInt();
	_entry->position.base = reader.value("position_base").toInt();
	_entry->position.secondary = reader.value("position_secondary").toInt();
	return _entry;
}
}
