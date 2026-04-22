#pragma once
#include "DataLayer/images/i_tile_groups_data_provider.h"
#include <QObject>
#include <QList>
#include <QPixmap>
#include <memory>
#include <optional>

class TexturesService;

class TilesService : public QObject {
	Q_OBJECT
public:
	explicit TilesService(
		TexturesService* texturesService,
		ITileGroupsDataProvider* tileGroupsDataProvider,
		QObject* parent = nullptr);
	~TilesService() override;

	// Получить все группы для текстуры
	QList<TileGroup> getGroups(const QString& texturePath) const;

	// Найти группу, содержащей указанный тайл
	std::optional<TileGroup> getGroupContainingTile(const QString& texturePath, int tileId) const;

	// Создать новую группу из выбранных тайлов
	QUuid createGroup(const QString& texturePath, const QString& name, const QList<int>& tileIds);

	// Обновить существующую группу
	bool updateGroup(const QString& texturePath, const TileGroup& group);

	// Удалить группу
	bool deleteGroup(const QUuid& groupId);

	// Удалить все группы для текстуры
	bool deleteGroupsForTexture(const QString& texturePath);

	// Загрузить метаданные тайлового набора
	std::optional<TileSetMetadata> getTileSetMetadata(const QString& texturePath) const;

	// Вернуть загруженные метаданные
	std::optional<TileSetMetadata> getCurrentTileSetMetadata() const;
	
	// Сохранить текущие выбранные тайлы
	void setSelectionTiles(const QList<int>& tileIds);

	// Вернуть текущие выбранные тайлы
	QList<int> getSelectionTiles() const;

	// Выдать тайловую карту
	std::optional<QPixmap> getTilemap(const QString& tag) const;

	// Вернуть название атласа
	QString getTileSetName() const;

signals:
	void groupsChanged(const QString& texturePath);
	void tilesSelectionChanged(const QList<int>& tileIds);

private:
	class Private;
	std::unique_ptr<Private> d;
};
