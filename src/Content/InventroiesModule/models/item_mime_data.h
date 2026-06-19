#pragma once
#include <QString>
#include <QUuid>

class Item;
class ItemEntity;
class ContainerItem;

class ItemMimeData {
public:
	QUuid id;
	QUuid entityId;
	QString name;
	int width;
	int height;
	int count;
	int maxStack;
	int type;
	int subType;
	int x;
	int y;

public:
	ItemMimeData();
	ItemMimeData(const ContainerItem& item);
	ItemMimeData(const Item& item);

	static ItemMimeData fromMimeData(const QByteArray& data);
	QByteArray toMimeData() const;

private:
	void setEntity(const ItemEntity& entity);
};
