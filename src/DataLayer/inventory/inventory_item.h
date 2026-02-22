#pragma once
#include "DataLayer/items/item.h"
#include <QString>
#include <list>
#include <map>
#include <memory>

class InventoryItem {
public:
	QString id;
	int x;
	int y;
	int count;
};

class Inventory {
public:
	QString id;
	QString name;
	int rows;
	int cols;
	std::list<InventoryItem> items;
};

