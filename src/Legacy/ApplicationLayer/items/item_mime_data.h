#pragma once
#include <QString>
#include <QUuid>

class Item;
class ItemEntity;
class InventoryItemHandler;
class EquipmentItemHandler;

enum class ItemOwner {
	Unknown,
	Inventory,
	Equipment,
};

class ItemMimeData {
public:
	QUuid id;
	QString name;
	QString entityId;
	int owner;
	int width;
	int height;
	int count;
	int maxStack;
	int type;
	int equipmentType;
	int x;
	int y;

public:
	ItemMimeData();
	ItemMimeData(const InventoryItemHandler& item);
	ItemMimeData(const EquipmentItemHandler& item);
	ItemMimeData(const Item& item);

	static ItemMimeData fromMimeData(const QByteArray& data);
	QByteArray toMimeData() const;

private:
	void setEntity(const ItemEntity& entity);
};
