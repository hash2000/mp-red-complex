#pragma once
#include <QString>
#include <QUuid>

class Item;

class ItemsDataWriter {
public:
	virtual ~ItemsDataWriter() = default;

	virtual bool saveItem(const QUuid& id, const Item& item) const = 0;
};
