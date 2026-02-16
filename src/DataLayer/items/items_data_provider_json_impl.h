#pragma once
#include "DataLayer/items/items_data_provider.h"
#include <QPixmap>
#include <memory>

class Resources;

class ItemsDataProviderJsonImpl : public ItemsDataProvider {
public:
	ItemsDataProviderJsonImpl(Resources* resources);
	~ItemsDataProviderJsonImpl() override;

	bool load(const QString& id, Item& item) const override;

private:
	QPixmap loadIcon(const Item& item) const;
	static QPixmap loadEmptyStubIcon(const QString& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
