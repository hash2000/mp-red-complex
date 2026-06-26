#pragma once 
#include "DataLayer/equipment/i_equipment_data_provider.h"
#include <memory>

class Resources;

class EquipmentDataProviderJsonImpl : public IEquipmentDataProvider {
public:
	EquipmentDataProviderJsonImpl(Resources* resources);
	~EquipmentDataProviderJsonImpl() override;

	bool loadEquipment(const QUuid& id, Equipment& equipment) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
