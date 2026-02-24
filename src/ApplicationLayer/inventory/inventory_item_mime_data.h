#pragma once
#include <QString>

class InventoryItemHandler;

class InventoryItemMimeData {
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

public:
	InventoryItemMimeData();
	InventoryItemMimeData(const InventoryItemHandler& item);

	static InventoryItemMimeData fromMimeData(const QByteArray& data);
};
