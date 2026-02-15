#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QWidget>

class InventoryGrid;

class StackSplitWidget : public QWidget {
	Q_OBJECT

public:
	explicit StackSplitWidget(const InventoryHandler& item, InventoryGrid* grid, QWidget* parent = nullptr);
	~StackSplitWidget() override;

	int selectedCount() const;
	const InventoryHandler& originalItem() const;
	void positionNearWidget(QWidget* targetWidget);

	// Создать временный предмет для дропа с выбранным количеством
	InventoryHandler createSplitItem() const;

signals:
	void splitConfirmed(int count);
	void splitCancelled();
	void splitDragStarted(const InventoryHandler& splitItem);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void leaveEvent(QEvent* event) override;
	bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
	void onSliderChanged(int value);
	void onLineEditTextChanged(const QString& text);
	void onStartDrag();

private:
	void setupUi();
	void setupEventHandling();

private:
	class Private;
	std::unique_ptr<Private> d;
};
