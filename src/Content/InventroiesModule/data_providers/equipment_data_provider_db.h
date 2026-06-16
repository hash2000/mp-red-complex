#pragma once 
#include "Content/InventroiesModule/data_providers/i_equipment_data_provider.h"
#include <memory>
#include <list>

class DatabasesService;

class EquipmentDataProviderDb : public IEquipmentDataProvider {
public:
	EquipmentDataProviderDb(DatabasesService* databasesService);
	~EquipmentDataProviderDb() override;

	std::shared_ptr<Equipment> get(const QUuid& id) override;
	bool save(const QUuid& id, const Equipment& equipment) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
