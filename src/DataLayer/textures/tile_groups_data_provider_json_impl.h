#pragma once
#include "DataLayer/textures/i_tile_groups_data_provider.h"
#include <memory>

class Resources;

class TileGroupsDataProviderJsonImpl : public ITileGroupsDataProvider {
public:
	TileGroupsDataProviderJsonImpl(Resources* resources);
	~TileGroupsDataProviderJsonImpl() override;

	QList<TileGroup> loadGroups(const QString& texturePath) const override;
	bool saveGroup(const QString& texturePath, const TileGroup& group, const TileSetMetadata& metadata) override;
	bool deleteGroup(const QUuid& groupId, const TileSetMetadata& metadata) override;
	bool deleteGroupsForTexture(const QString& texturePath) override;
	std::optional<TileSetMetadata> loadTileSetMetadata(const QString& path) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
