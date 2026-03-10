#pragma once
#include "DataLayer/equipment/equipment_data_writer.h"
#include <QString>
#include <memory>

class Resources;

class EquipmentDataWriterJsonImpl : public EquipmentDataWriter {
public:
	EquipmentDataWriterJsonImpl(Resources* resources);
	~EquipmentDataWriterJsonImpl() override;

	bool saveEquipment(const QUuid& id, const Equipment& equipment) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
