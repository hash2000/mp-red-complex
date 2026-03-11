#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>
#include <memory>

class ItemsService;
class QLabel;
class QTableWidget;
class QPushButton;

/// Виджет создания нового предмета
/// Отображает информацию о предмете и позволяет перетащить его в инвентарь/экипировку
class ItemCreateWidget : public QWidget {
	Q_OBJECT
public:
	explicit ItemCreateWidget(
		const ItemEntity& entity,
		ItemsService* itemsService,
		const QString& targetInventoryId,
		QWidget* parent = nullptr);
	~ItemCreateWidget() override;

signals:
	/// Сигнал о создании предмета (когда пользователь подтверждает создание)
	void itemCreated(const QString& entityId);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	void setupLayout();
	void updateItemImage();
	void populateParamsTable();

	class Private;
	std::unique_ptr<Private> d;
};
