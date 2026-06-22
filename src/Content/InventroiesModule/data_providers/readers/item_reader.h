#pragma once
#include <memory>

class SQLiteReader;
class Item;

namespace DataProviders::Readers {
class ItemReader {
public:
	ItemReader(std::shared_ptr<Item> item = std::shared_ptr<Item>());
	std::shared_ptr<Item> read(SQLiteReader& reader);
private:
	std::shared_ptr<Item> _item;
};
}
