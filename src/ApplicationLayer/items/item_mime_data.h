#pragma once
#include <QString>

class ItemEntity;
class InventoryItemHandler;
class EquipmentItemHandler;

enum class ItemOwner {
	Inventory,
	Equipment,
};

class ItemMimeData {
public:
	QString id;
	QString name;
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

	static ItemMimeData fromMimeData(const QByteArray& data);
	QByteArray toMimeData() const;

private:
	void setEntity(const ItemEntity& entity);
};
