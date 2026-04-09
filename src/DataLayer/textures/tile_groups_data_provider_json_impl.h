#pragma once
#include "DataLayer/textures/i_tile_groups_data_provider.h"
#include <memory>

class Resources;

class TileGroupsDataProviderJsonImpl : public ITileGroupsDataProvider {
public:
	TileGroupsDataProviderJsonImpl(Resources* resources);
	~TileGroupsDataProviderJsonImpl() override;

	QList<TileGroup> loadGroups(const QString& texturePath) const override;
	bool saveGroup(const TileGroup& group) override;
	bool deleteGroup(const QUuid& groupId) override;
	bool deleteGroupsForTexture(const QString& texturePath) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
