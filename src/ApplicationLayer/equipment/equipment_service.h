#pragma once
#include <QObject>
#include <memory>

class ItemsService;
class EquipmentDataProvider;

class EquipmentService : public QObject {
	Q_OBJECT
public:
	EquipmentService(EquipmentDataProvider* dataProvider, ItemsService* itemsService, QObject* parent = nullptr);
	~EquipmentService() override;

	bool load(const QUuid& id, bool loadIfNotExists);

private:
	class Private;
	std::unique_ptr<Private> d;
};
