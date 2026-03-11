#pragma once
#include <QString>

class Item;

class ItemsDataWriter {
public:
	virtual ~ItemsDataWriter() = default;

	virtual bool saveItem(const QString& id, const Item& item) const = 0;
};
