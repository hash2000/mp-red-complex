#pragma once
#include <QString>

enum class ItemResourceType;
enum class ItemType;
enum class ItemSubType;
enum class ItemRarityType;

namespace Inventory::Entities {
ItemResourceType resourceType(const QString& type);
ItemType itemType(const QString& type);
ItemSubType itemSubType(const QString& type);
QString resourceTypeToString(ItemResourceType type);
QString itemTypeToString(ItemType type);
QString itemSubTypeToString(ItemSubType type);
QString itemRarityTypeToString(ItemRarityType type);
}
