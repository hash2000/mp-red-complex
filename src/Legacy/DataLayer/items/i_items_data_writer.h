#pragma once
#include <QString>
#include <QUuid>

class Item;

class IItemsDataWriter {
public:
	virtual ~IItemsDataWriter() = default;

	virtual bool saveItem(const QUuid& id, const Item& item) const = 0;
};
