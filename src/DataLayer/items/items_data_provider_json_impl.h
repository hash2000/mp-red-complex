#pragma once
#include "DataLayer/items/items_data_provider.h"
#include <QPixmap>
#include <memory>

class Resources;

class ItemsDataProviderJsonImpl : public ItemsDataProvider {
public:
	ItemsDataProviderJsonImpl(Resources* resources);
	~ItemsDataProviderJsonImpl() override;

	bool loadEntitiesIds(std::list<QString>& list) const override;
	bool loadEntity(const QString& id, ItemEntity& entity) const override;
	bool loadItem(const QString& id, Item& item) const override;

private:
	QPixmap loadIcon(const ItemEntity& item) const;
	static QPixmap loadEmptyStubIcon(const QString& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
