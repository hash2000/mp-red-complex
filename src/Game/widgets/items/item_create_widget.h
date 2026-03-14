#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>
#include <memory>

class ItemsService;
class InventoriesService;
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
		InventoriesService* inventoriesService,
		QWidget* parent = nullptr);
	~ItemCreateWidget() override;

	/// Установить список доступных инвентарей для выбора
	/// @param inventoryIds Список идентификаторов доступных инвентарей
	/// @param preselectedInventoryId Предварительно выбранный инвентарь (если есть)
	void setAvailableInventories(const QStringList& inventoryIds, const QString& preselectedInventoryId = QString());

	/// Получить текущий выбранный инвентарь
	QString selectedInventoryId() const;

signals:
	/// Сигнал о создании предмета (когда пользователь подтверждает создание)
	/// @param entityId ID сущности предмета
	/// @param count Количество предметов
	/// @param inventoryId ID инвентаря, в который создаётся предмет
	void itemCreated(const QString& entityId, int count, const QString& inventoryId);

private slots:
	void onInventorySelectionRequested();
	void onItemSelected();

private:
	void setupLayout();
	void updateItemImage();
	void populateParamsTable();
	void updateInventoryDisplay();
	void showInventorySelectionDialog();
	QString getInventoryDisplayName(const QString& inventoryId) const;

	class Private;
	std::unique_ptr<Private> d;
};
