#pragma once
#include <QString>
#include <QList>
#include <QUuid>
#include <optional>

// Метаданные тайлового набора
struct TileSetMetadata {
	struct {
		int x = 64;
		int y = 64;
	} gridSize;
	QString fileName;
};

// Структура группы тайлов
struct TileGroup {
	QUuid id;              // Уникальный ID группы
	QString name;          // Отображаемое имя группы
	QList<int> tileIds;    // Список ID тайлов в группе
};

// Интерфейс провайдера данных для групп тайлов
class ITileGroupsDataProvider {
public:
	virtual ~ITileGroupsDataProvider() = default;

	// Загрузить все группы для указанного текстуры
	virtual QList<TileGroup> loadGroups(const QString& texturePath) const = 0;

	// Сохранить группу
	virtual bool saveGroup(const QString& texturePath, const TileGroup& group, const TileSetMetadata& metadata) = 0;

	// Удалить группу по ID
	virtual bool deleteGroup(const QUuid& groupId, const TileSetMetadata& metadata) = 0;

	// Удалить все группы для текстуры
	virtual bool deleteGroupsForTexture(const QString& texturePath) = 0;

	// Загрузить метаданные тайлового набора (если type == TileSets)
	virtual std::optional<TileSetMetadata> loadTileSetMetadata(const QString& path) const = 0;
};
