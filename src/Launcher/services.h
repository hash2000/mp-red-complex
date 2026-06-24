#pragma once
#include <QObject>
#include <memory>

class Resources;
class TimeService;
class WorldService;
class ItemsService;
class UsersService;
class ImagesService;
class TilesSelectorService;
class TexturesService;
class ShadersService;
class MaterialsService;
class HighlightingPluginManager;
class DatabasesService;

class Services: public QObject {
	Q_OBJECT
public:
	explicit Services(Resources* resources);
	~Services();

	TimeService* timeService() const;
	WorldService* worldService() const;
	ItemsService* itemsService() const;
	UsersService* usersService() const;
	ImagesService* imagesService() const;
	TilesSelectorService* tilesSelectorService() const;
	TexturesService* texturesService() const;
	MaterialsService* materialsService() const;
	HighlightingPluginManager* highlightingPluginManager() const;
	DatabasesService* databasesService() const;

	// для каждого контекста opengl должен быть свой
	std::unique_ptr<ShadersService> shadersService() const;

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
