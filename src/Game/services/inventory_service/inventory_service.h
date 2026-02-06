#pragma once
#include "DataLayer/data_serializer.h"
#include <QObject>
#include <memory>

class InventoryService : public QObject {
	Q_OBJECT
public:InventoryService(QObject* parent = nullptr);
	~InventoryService() override;

public slots:
	void onSave();
	void onLoad();

private:
	class Private;
	std::unique_ptr<Private> d;
};
