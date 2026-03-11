#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>
#include <memory>

class ItemsService;
class QLabel;
class QTableWidget;
class QPushButton;

/// Виджет создания нового предмета
/// Отображает информацию о предмете и позволяет создать его в инвентаре
class ItemCreateWidget : public QWidget {
	Q_OBJECT
public:
	explicit ItemCreateWidget(
		const ItemEntity& entity,
		ItemsService* itemsService,
		QWidget* parent = nullptr);
	~ItemCreateWidget() override;

signals:
	/// Сигнал о создании предмета (когда пользователь подтверждает создание)
	void itemCreated(const QString& entityId);

private:
	void setupLayout();
	void updateItemImage();
	void populateParamsTable();

	class Private;
	std::unique_ptr<Private> d;
};
