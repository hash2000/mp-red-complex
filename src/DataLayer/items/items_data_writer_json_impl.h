#pragma once
#include "DataLayer/items/i_items_data_writer.h"
#include <memory>

class Resources;

class ItemsDataWriterJsonImpl : public IItemsDataWriter {
public:
	ItemsDataWriterJsonImpl(Resources* resources);
	~ItemsDataWriterJsonImpl() override;

	bool saveItem(const QUuid& id, const Item& item) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
