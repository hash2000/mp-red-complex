#pragma once
#include <QUuid>
#include <optional>

class Equipment;

class IEquipmentDataProvider {
public:
	virtual ~IEquipmentDataProvider() = default;

	virtual std::shared_ptr<Equipment> equipment(const QUuid& id) = 0;
	virtual bool save(const QUuid& id, const Equipment& equipment) const = 0;
};
