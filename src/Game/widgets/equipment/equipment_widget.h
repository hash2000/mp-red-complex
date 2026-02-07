#pragma once
#include "Game/widgets/equipment/equipment_slot_widget.h"
#include <QWidget>
#include <optional>
#include <memory>

class EquipmentWidget : public QFrame {
	Q_OBJECT

public:
	explicit EquipmentWidget(QWidget* parent = nullptr);
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
	void setupLayout();
	EquipmentSlot* findCompatibleSlot(const EquipmentItem& item) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
