#pragma once
#include <QString>
#include <list>

struct InventoryItemRaw {
	QString entityId;
	QString id;
	int x;
	int y;
};

struct InventoryRaw {
	QString id;
	QString name;
	int rows;
	int cols;
	std::list<InventoryItemRaw> items;
};
