#include "Content/InventroiesModule/data_providers/readers/item_reader.h"
#include "Content/InventroiesModule/models/item.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"


namespace DataProviders::Readers {
ItemReader::ItemReader(std::shared_ptr<Item> item)
	: _item(item) {
	if (!_item) {
		_item = std::make_shared<Item>();
	}
}

std::shared_ptr<Item> ItemReader::read(SQLiteReader& reader) {
	_item->id = QUuid::fromString(reader.value("id").toString());
	_item->entityId = reader.value("entity_id").toString();
	_item->containerId = QUuid::fromString(reader.value("container_id").toString());
	_item->level = reader.value("level").toInt();
	_item->count = reader.value("count").toInt();
	_item->position.base = reader.value("position_base").toInt();
	_item->position.secondary = reader.value("position_secondary").toInt();
	return _item;
}
}
