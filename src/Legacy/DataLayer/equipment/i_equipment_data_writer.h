#pragma once
#include <QUuid>

class Equipment;

class IEquipmentDataWriter {
public:
	virtual ~IEquipmentDataWriter() = default;

	virtual bool saveEquipment(const QUuid& id, const Equipment& equipment) const = 0;
};
