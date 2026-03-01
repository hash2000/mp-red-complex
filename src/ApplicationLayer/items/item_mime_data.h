#pragma once
#include <QString>

class InventoryItemHandler;

class ItemMimeData {
public:
	QString id;
	QString name;
	int x;
	int y;
	int width;
	int height;
	int count;
	int maxStack;
	int type;
	int equipmentType;

public:
	ItemMimeData();
	ItemMimeData(const InventoryItemHandler& item);

	static ItemMimeData fromMimeData(const QByteArray& data);
};
