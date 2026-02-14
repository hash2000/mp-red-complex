#pragma once
#include <QObject>
#include <memory>

class InventoriesController : public QObject {
	Q_OBJECT
public:
	InventoriesController();
	~InventoriesController() override;

	bool postInventoryLoad(const QUuid& id);
	void postInventoryLoaded(const QUuid& id);
	void postInventoryUnloaded(const QUuid& id);

signals:
	void inventoryLoaded(const QUuid& id);
	void inventoryClosed(const QUuid& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
