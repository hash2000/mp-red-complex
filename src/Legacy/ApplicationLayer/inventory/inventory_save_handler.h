#pragma once
#include <QObject>
#include <QUuid>
#include <memory>

class InventoryService;
class IInventoryDataWriter;

/**
 * @brief Обработчик сохранения инвентаря
 * 
 * Слушает сигналы изменений от InventoryService и сохраняет
 * состояние инвентаря через IInventoryDataWriter
 */
class InventorySaveHandler : public QObject {
	Q_OBJECT
public:
	InventorySaveHandler(
		InventoryService* inventoryService,
		IInventoryDataWriter* dataWriter,
		const QUuid& inventoryId,
		QObject* parent = nullptr);

	~InventorySaveHandler() override;

private slots:
	void onItemChanged();

private:
	class Private;
	std::unique_ptr<Private> d;
};
