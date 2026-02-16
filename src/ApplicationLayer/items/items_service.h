#pragma once
#include "DataLayer/items/item.h"
#include <QObject>
#include <memory>

class ItemsService : public QObject {
	Q_OBJECT
public:
	ItemsService();
	~ItemsService() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
