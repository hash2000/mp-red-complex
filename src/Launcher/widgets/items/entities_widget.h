#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>
#include <memory>

class ItemsService;
class InventoriesService;

class EntitiesWidget : public QWidget {
	Q_OBJECT
public:
	explicit EntitiesWidget(
		ItemsService* service,
		InventoriesService* inventoriesService,
		QWidget* parent = nullptr);
	~EntitiesWidget() override;

	void addItemEntity(const ItemEntity& entity);

	void clear();

	// Установка доступных инвентарей и предварительный выбор
	void setAvailableInventories(const QStringList& inventoryIds, const QString& preselectedInventoryId = QString());

	// Получение текущего выбранного инвентаря
	QString selectedInventoryId() const;

	// Установка сервиса инвентарей (может вызываться позже, после создания виджета)
	void setInventoriesService(InventoriesService* service);

signals:
	// Запрос на создание предмета с указанием инвентаря
	void itemCreateRequested(const QString& itemId, const QString& inventoryId);

	// Запрос диалога выбора инвентаря (когда нужно показать диалог пользователю)
	void inventorySelectionRequested();

	// Сигнал об изменении выбранного инвентаря
	void inventorySelectionChanged(const QString& inventoryId);

private slots:
	void onInventorySelectionRequested();
	void showInventorySelectionDialog();

private:
	void updateInventoryDisplay();
	QString getInventoryDisplayName(const QString& inventoryId) const;

	class Private;
	std::unique_ptr<Private> d;
};
