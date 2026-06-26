#pragma once
#include <QString>
#include <memory>

class SQLiteReader;
class ItemEntity;
enum class ItemResourceType;
enum class ItemType;
enum class ItemSubType;


namespace Inventory::Readers {
class EntityReader {
public:
	EntityReader(std::shared_ptr<ItemEntity> entry = std::shared_ptr<ItemEntity>());
	std::shared_ptr<ItemEntity> read(SQLiteReader& reader);

private:
	std::shared_ptr<ItemEntity> _entry;
};
}
