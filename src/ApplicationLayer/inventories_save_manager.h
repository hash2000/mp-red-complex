#pragma once
#include <QObject>
#include <memory>

class InventoriesService;
class IInventoryDataWriter;
class IEquipmentDataWriter;

/**
 * @brief Менеджер сохранения инвентарей и экипировок
 * 
 * Подключается к сигналу save() из Services и сохраняет
 * все инвентари и экипировки через DataWriter'ы
 */
class InventoriesSaveManager : public QObject {
	Q_OBJECT
public:
	InventoriesSaveManager(
		InventoriesService* inventoriesService,
		IInventoryDataWriter* inventoryDataWriter,
		IEquipmentDataWriter* equipmentDataWriter,
		QObject* parent = nullptr);

	~InventoriesSaveManager() override;

public slots:
	void saveAll();

private:
	class Private;
	std::unique_ptr<Private> d;
};
