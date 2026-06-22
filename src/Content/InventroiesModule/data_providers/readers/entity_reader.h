#pragma once
#include <QString>
#include <memory>

class SQLiteReader;
class ItemEntity;
enum class ItemResourceType;
enum class ItemType;
enum class ItemSubType;


namespace DataProviders::Readers {
class EntityReader {
public:
	EntityReader(std::shared_ptr<ItemEntity> entity = std::shared_ptr<ItemEntity>());
	std::shared_ptr<ItemEntity> read(SQLiteReader& reader);

private:
	ItemResourceType resourceType(const QString& type);
	ItemType itemType(const QString& type);
	ItemSubType itemSubType(const QString& type);
private:
	std::shared_ptr<ItemEntity> _entity;
};
}
