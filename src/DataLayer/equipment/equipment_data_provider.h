#pragma once
#include <QUuid>

class Equipment;

class EquipmentDataProvider {
public:
	virtual ~EquipmentDataProvider() = default;

	virtual bool loadEquipment(const QUuid& id, Equipment& equipment) = 0;

};
