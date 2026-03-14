#pragma once
#include "DataLayer/items/items_data_writer.h"
#include <memory>

class Resources;

class ItemsDataWriterJsonImpl : public ItemsDataWriter {
public:
	ItemsDataWriterJsonImpl(Resources* resources);
	~ItemsDataWriterJsonImpl() override;

	bool saveItem(const QUuid& id, const Item& item) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
