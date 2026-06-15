#pragma once
#include <QString>
#include <QUuid>
#include <list>

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

