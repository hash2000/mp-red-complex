#pragma once
#include "DataLayer/items/item.h"
#include <QString>
#include <QUuid>
#include <list>
#include <map>
#include <memory>

class InventoryItem {
public:
	QUuid id;
	int x;
	int y;
	int count;
};

class Inventory {
public:
	QUuid id;
	QString name;
	int rows;
	int cols;
	std::list<InventoryItem> items;
};

