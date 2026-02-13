#pragma once
#include <QObject>
#include <memory>

class InventoriesService;

class InventoriesController : public QObject {
	Q_OBJECT
public:
	InventoriesController(InventoriesService* service);
	~InventoriesController() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
