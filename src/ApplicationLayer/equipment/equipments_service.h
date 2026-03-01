#pragma once
#include <QObject>

class ItemsService;
class EquipmentDataProvider;
class EquipmentService;

class EquipmentsService : public QObject {
	Q_OBJECT
public:
	EquipmentsService(EquipmentDataProvider* dataProvider, ItemsService* itemsService, QObject* parent = nullptr);
	~EquipmentsService() override;

	EquipmentService* equipment(const QUuid& id, bool loadIfNotExists);

private:
	class Private;
	std::unique_ptr<Private> d;
};
