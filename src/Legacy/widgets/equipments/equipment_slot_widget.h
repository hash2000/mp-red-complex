#pragma once
#include <QLabel>
#include <QFrame>
#include <memory>

class EquipmentWidget;
class EquipmentService;
class ItemMimeData;
class ContainerItem;
enum class ItemSlotType;

class EquipmentSlot : public QLabel {
  Q_OBJECT
public:
  explicit EquipmentSlot(
		EquipmentService* equipmentService,
		ItemSlotType type,
		EquipmentWidget* parentWidget,
		QWidget* parent = nullptr);
  ~EquipmentSlot() override;

	bool isOccupied() const;
	bool isHighlighted() const;
  void setHighlighted(bool highlighted);

	ItemSlotType slotType() const;
	void clearItem();
	void setItem(std::shared_ptr<ContainerItem> item);

signals:
	void containerOpened(const ItemMimeData& container);

protected:
  void mousePressEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
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
