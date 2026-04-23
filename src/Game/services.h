#pragma once
#include <QObject>
#include <memory>

class Resources;
class TimeService;
class WorldService;
class InventoriesService;
class ItemsService;
class InventoryLoader;
class IInventoryRepository;
class IUsersDataProvider;
class IImagesDataProvider;
class ITileGroupsDataProvider;
class UsersService;
class ImagesService;
class TilesService;
class MapService;
class IMapDataProvider;
class TexturesService;

class Services: public QObject {
	Q_OBJECT
public:
	explicit Services(Resources* resources);
	~Services();

	TimeService* timeService() const;
	WorldService* worldService() const;
	ItemsService* itemsService() const;
	InventoriesService* inventoriesService() const;
	InventoryLoader* inventoryLoader() const;
	IInventoryRepository* inventoryRepository() const;
	UsersService* usersService() const;
	ImagesService* imagesService() const;
	TilesService* tilesService() const;
	MapService* mapService() const;
	TexturesService* texturesService() const;

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
