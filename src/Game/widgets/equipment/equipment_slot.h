#pragma once
#include "Game/widgets/equipment/equipment_item.h"
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
  enum class SlotType {
		Head,
		Body,
		WeaponLeft,
		WeaponRight,
		GlovesLeft,
		GlovesRight,
		Boots,
		RingLeft,
		RingRight,
		Amulet,
  };

  explicit EquipmentSlot(SlotType type, EquipmentWidget* parentWidget, QWidget* parent = nullptr);
  ~EquipmentSlot() override;

	bool isOccupied() const;
	bool isHighlighted() const;
  void setHighlighted(bool highlighted);

	const std::optional<Item>& item() const;
	SlotType slotType() const;

  // Публичный интерфейс для программного управления
  bool setItem(const Item& item);
  void clearItem();
  bool canAcceptItem(const Item& item) const;

signals:
  void itemEquipped(const Item& item, EquipmentSlot* slot);
  void itemRemoved(const Item& item, EquipmentSlot* slot);
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
