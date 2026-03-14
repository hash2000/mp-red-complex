#pragma once
#include <QWidget>
#include <QLabel>
#include <QFrame>
#include <QMap>
#include <optional>
#include <memory>

class EquipmentWidget;
class EquipmentService;
class EquipmentItemHandler;
enum class EquipmentSlotType;

class EquipmentSlot : public QLabel {
  Q_OBJECT
public:
  explicit EquipmentSlot(EquipmentService* equipmentService, EquipmentSlotType type, EquipmentWidget* parentWidget, QWidget* parent = nullptr);
  ~EquipmentSlot() override;

	bool isOccupied() const;
	bool isHighlighted() const;
  void setHighlighted(bool highlighted);

	EquipmentSlotType slotType() const;
	void clearItem();
	void setItem(const EquipmentItemHandler& item);

  // Публичный интерфейс для программного управления
  //bool setItem(const EquipmentItem& item);
  //bool canAcceptItem(const EquipmentItem& item) const;

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
