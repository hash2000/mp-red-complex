#pragma once
#include "DataLayer/equipment/equipment_item.h"
#include "DataLayer/equipment/equipment_slot.h"
#include <QWidget>
#include <QLabel>
#include <QFrame>
#include <QMap>
#include <optional>
#include <memory>

class EquipmentWidget;

class EquipmentSlot : public QLabel {
  Q_OBJECT
public:
  explicit EquipmentSlot(EquipmentSlotType type, EquipmentWidget* parentWidget, QWidget* parent = nullptr);
  ~EquipmentSlot() override;

	bool isOccupied() const;
	bool isHighlighted() const;
  void setHighlighted(bool highlighted);

	const std::optional<EquipmentItem>& item() const;
	EquipmentSlotType slotType() const;

  // Публичный интерфейс для программного управления
  bool setItem(const EquipmentItem& item);
  void clearItem();
  bool canAcceptItem(const EquipmentItem& item) const;

signals:
  void itemEquipped(const EquipmentItem& item, EquipmentSlot* slot);
  void itemRemoved(const EquipmentItem& item, EquipmentSlot* slot);
  void slotClicked(EquipmentSlot* slot);

protected:
  void mousePressEvent(QMouseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragLeaveEvent(QDragLeaveEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

private:
  void updateVisualState();
  void startDrag();

private:
	class Private;
	std::unique_ptr<Private> d;
};
