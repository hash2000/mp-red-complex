#pragma once
#include "DataLayer/equipment/equipment.h"
#include <QObject>
#include <memory>

class ItemsService;
class Equipment;
class EquipmentItem;
class ItemMimeData;

class EquipmentService : public QObject {
	Q_OBJECT
public:
	EquipmentService(ItemsService* itemsService, QObject* parent = nullptr);
	~EquipmentService() override;

	bool load(const Equipment& equipment);

	bool canAcceptItem(const ItemMimeData& item, EquipmentSlotType slot) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
