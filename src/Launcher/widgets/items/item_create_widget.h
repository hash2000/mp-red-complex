#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>
#include <memory>

class ItemsService;
class QLabel;
class QTableWidget;
class QPushButton;
class QSpinBox;

/// Виджет создания нового предмета
/// Отображает информацию о предмете и позволяет создать его в инвентаре
class ItemCreateWidget : public QWidget {
	Q_OBJECT
public:
	explicit ItemCreateWidget(
		const ItemEntity& entity,
		ItemsService* itemsService,
		const QString& inventoryId,
		QWidget* parent = nullptr);
	~ItemCreateWidget() override;

signals:
	/// Сигнал о создании предмета (когда пользователь подтверждает создание)
	/// @param entityId ID сущности предмета
	/// @param count Количество предметов
	/// @param inventoryId ID инвентаря, в который создаётся предмет
	void itemCreated(const QString& entityId, int count, const QString& inventoryId);

private:
	void setupLayout();
	void updateItemImage();
	void populateParamsTable();

	class Private;
	std::unique_ptr<Private> d;
};
