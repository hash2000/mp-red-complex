#pragma once
#include <QUuid>
#include <QString>
#include <QPoint>
#include <list>
#include <memory>

class Item;
enum class ItemSlotType;

class ContainerItem {
public:
	int position_base;
	int position_secondary;
	int count;
	std::shared_ptr<Item> item;

public:
	bool compare(const ContainerItem& item) const;
	bool canMergeWith(const ContainerItem& other) const;
	QPoint slotToPosition() const;
	static QPoint convertSlotToPosition(ItemSlotType slot);
	static ItemSlotType convertPositionToSlot(int col, int row);
};

class Container {
public:
	QUuid id;
	QString name;
	std::list<std::shared_ptr<ContainerItem>> items;
};
