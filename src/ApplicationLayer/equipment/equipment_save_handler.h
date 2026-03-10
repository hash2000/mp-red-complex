#pragma once
#include <QObject>
#include <QUuid>
#include <memory>

class EquipmentService;
class EquipmentDataWriter;

/**
 * @brief Обработчик сохранения экипировки
 * 
 * Слушает сигналы изменений от EquipmentService и сохраняет
 * состояние экипировки через EquipmentDataWriter
 */
class EquipmentSaveHandler : public QObject {
	Q_OBJECT
public:
	EquipmentSaveHandler(
		EquipmentService* equipmentService,
		EquipmentDataWriter* dataWriter,
		const QUuid& equipmentId,
		QObject* parent = nullptr);

	~EquipmentSaveHandler() override;

private slots:
	void onItemChanged();

private:
	class Private;
	std::unique_ptr<Private> d;
};
