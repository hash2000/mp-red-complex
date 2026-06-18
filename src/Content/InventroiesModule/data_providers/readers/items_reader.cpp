#include "Content/InventroiesModule/data_providers/readers/items_reader.h"
#include "Content/InventroiesModule/models/item.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"


namespace DataProviders::Readers {
std::shared_ptr<Item> ItemsReader::read(SQLiteReader& reader) const {
	return std::shared_ptr<Item>();
}
}
