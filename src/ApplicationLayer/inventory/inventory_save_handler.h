#pragma once
#include <QObject>
#include <QUuid>
#include <memory>

class InventoryService;
class InventoryDataWriter;

/**
 * @brief Обработчик сохранения инвентаря
 * 
 * Слушает сигналы изменений от InventoryService и сохраняет
 * состояние инвентаря через InventoryDataWriter
 */
class InventorySaveHandler : public QObject {
	Q_OBJECT
public:
	InventorySaveHandler(
		InventoryService* inventoryService,
		InventoryDataWriter* dataWriter,
		const QUuid& inventoryId,
		QObject* parent = nullptr);

	~InventorySaveHandler() override;

private slots:
	void onItemChanged();

private:
	class Private;
	std::unique_ptr<Private> d;
};
