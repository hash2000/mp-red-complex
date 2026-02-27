#pragma once
#include "ApplicationLayer/equipment/equipment_item_handler.h"
#include <QFrame>
#include <QWidget>
#include <optional>
#include <memory>

class EquipmentSlot;
class EquipmentService;

class EquipmentWidget : public QFrame {
	Q_OBJECT

public:
	explicit EquipmentWidget(EquipmentService* equipmentService, QWidget* parent = nullptr);
	~EquipmentWidget() override;

	// Получить предмет из слота
	std::optional<EquipmentItem> getItem(EquipmentSlotType slot) const;

	// Программное снаряжение/снятие
	bool equipItem(const EquipmentItem& item);
	void unequipItem(EquipmentSlotType slot);

	// Очистить всё снаряжение
	void clearAll();

	// Получить все слоты для интеграции с инвентарем
	QMap<EquipmentSlotType, EquipmentSlot*> allSlots() const;

signals:
	void itemEquipped(const EquipmentItem& item, EquipmentSlotType slot);
	void itemUnequipped(const EquipmentItem& item, EquipmentSlotType slot);

private:
	class Private;
	std::unique_ptr<Private> d;
};
