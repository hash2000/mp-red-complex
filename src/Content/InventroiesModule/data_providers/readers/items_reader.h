#pragma once
#include <memory>

class SQLiteReader;
class Item;

namespace DataProviders::Readers {
class ItemsReader {
public:
	std::shared_ptr<Item> read(SQLiteReader& reader) const;
};
}
