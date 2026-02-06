#pragma once
#include "Game/widgets/equipment/equipment_slot.h"
#include <QWidget>
#include <optional>
#include <memory>

class EquipmentWidget : public QFrame {
	Q_OBJECT

public:
	explicit EquipmentWidget(QWidget* parent = nullptr);
	~EquipmentWidget() override;

	// Получить предмет из слота
	std::optional<Item> getItem(EquipmentSlot::SlotType slot) const;

	// Программное снаряжение/снятие
	bool equipItem(const Item& item);
	void unequipItem(EquipmentSlot::SlotType slot);

	// Очистить всё снаряжение
	void clearAll();

	// Получить все слоты для интеграции с инвентарем
	QMap<EquipmentSlot::SlotType, EquipmentSlot*> allSlots() const;

signals:
	void itemEquipped(const Item& item, EquipmentSlot::SlotType slot);
	void itemUnequipped(const Item& item, EquipmentSlot::SlotType slot);

private:
	void setupLayout();
	EquipmentSlot* findCompatibleSlot(const Item& item) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
