#pragma once
#include <QUuid>

class Equipment;

class EquipmentDataWriter {
public:
	virtual ~EquipmentDataWriter() = default;

	virtual bool saveEquipment(const QUuid& id, const Equipment& equipment) const = 0;
};
