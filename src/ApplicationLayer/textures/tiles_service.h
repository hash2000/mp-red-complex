#pragma once
#include "DataLayer/textures/i_tile_groups_data_provider.h"
#include <QObject>
#include <memory>
#include <QList>
#include <optional>

class TilesService : public QObject {
	Q_OBJECT
public:
	explicit TilesService(
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
	std::optional<TileSetMetadata> getTileSetMetadata(const QString& path) const;

	// Сохранить текущие выбранные тайлы
	void setSelectionTiles(const QList<int>& tileIds);

	// Вернуть текущие выбранные тайлы
	QList<int> getSelectionTiles() const;

signals:
	void groupsChanged(const QString& texturePath);

private:
	class Private;
	std::unique_ptr<Private> d;
};
