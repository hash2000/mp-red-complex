#pragma once
#include "DataLayer/maps/i_map_data_provider.h"
#include <memory>

class Resources;

class MapDataProviderJsonImpl : public IMapDataProvider {
public:
    MapDataProviderJsonImpl(Resources* resources);
    ~MapDataProviderJsonImpl() override;

    std::optional<MapMetadata> loadMapMetadata(const QString& mapName) const override;
    bool saveMapMetadata(const QString& mapName, const MapMetadata& metadata) override;
    bool deleteMap(const QString& mapName) override;
    QList<QString> getAvailableMaps() const override;

private:
    class Private;
    std::unique_ptr<Private> d;
};
