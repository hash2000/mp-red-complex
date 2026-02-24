#pragma once
#include <QObject>
#include <memory>

class Resources;
class TimeService;
class WorldService;
class InventoriesService;
class ItemsService;

class Services: public QObject {
	Q_OBJECT
public:
	explicit Services(Resources* resources);
	~Services();

	TimeService* timeService() const;

	WorldService* worldService() const;

	InventoriesService* inventoriesService() const;

	ItemsService* itemsService() const;

	void run();

	void postLoadEvent();
	void postSaveEvent();

signals:
	void load();
	void save();

private:
	class Private;
	std::unique_ptr<Private> d;
};
