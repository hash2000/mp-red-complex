#pragma once
#include "Content/InventroiesModule/data_providers/i_entities_data_provider.h"
#include <QPixmap>
#include <QUuid>
#include <memory>

class DatabasesService;

class EntitiesDataProviderDb : public IEntitiesDataProvider {
public:
	EntitiesDataProviderDb(DatabasesService* databasesService);
	~EntitiesDataProviderDb() override;

	std::list<std::shared_ptr<ItemEntity>> entities() const override;
	std::shared_ptr<ItemEntity> entity(const QString& id) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
