#pragma once
#include <QUuid>

class Equipment;

class IEquipmentDataProvider {
public:
	virtual ~IEquipmentDataProvider() = default;

	virtual bool loadEquipment(const QUuid& id, Equipment& equipment) = 0;

};
