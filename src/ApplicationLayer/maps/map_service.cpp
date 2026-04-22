#include "ApplicationLayer/maps/map_service.h"
#include "ApplicationLayer/textures/tiles_service.h"
#include "ApplicationLayer/textures/images_service.h"
#include "DataLayer/maps/i_map_data_provider.h"
#include <QUuid>
#include <QFileInfo>

class MapService::Private {
public:
  Private(MapService* parent) : q(parent) {}
  MapService* q;
  IMapDataProvider* mapDataProvider = nullptr;
  TilesService* tilesService = nullptr;
  ImagesService* ImagesService = nullptr;

  std::optional<QString> currentMapName;
  mutable std::optional<MapMetadata> currentMetadata;
};

MapService::MapService(
  TilesService* tilesService,
  ImagesService* ImagesService,
  IMapDataProvider* mapDataProvider,
  QObject* parent)
  : QObject(parent)
  , d(std::make_unique<Private>(this)) {
  d->mapDataProvider = mapDataProvider;
  d->tilesService = tilesService;
  d->ImagesService = ImagesService;
}

MapService::~MapService() = default;

std::optional<MapMetadata> MapService::loadMapMetadata(const QString& mapName) const {
	if (d->currentMapName == mapName && d->currentMetadata.has_value()) {
		return d->currentMetadata;
	}

	d->currentMetadata = d->mapDataProvider->loadMapMetadata(mapName);
	if (d->currentMetadata.has_value()) {
		d->currentMapName = mapName;
	}
	else {
		d->currentMapName = std::nullopt;
	}

	return d->currentMetadata;
}

std::optional<QString> MapService::getCurrentMapName() const {
	return d->currentMapName;
}

std::optional<MapMetadata> MapService::getCurrentMapMetadata() const {
  return d->currentMetadata;
}

bool MapService::saveMapMetadata(const QString& mapName, const MapMetadata& metadata) {
  const bool success = d->mapDataProvider->saveMapMetadata(mapName, metadata);
  if (success) {
		d->currentMapName = mapName;
		d->currentMetadata = metadata;
  }
  return success;
}

bool MapService::deleteMap(const QString& mapName) {
  const bool success = d->mapDataProvider->deleteMap(mapName);
  if (success && d->currentMapName == mapName) {
		d->currentMapName = std::nullopt;
		d->currentMetadata = std::nullopt;
		emit currentMapChanged("");
  }
  return success;
}

QList<QString> MapService::getAvailableMaps() const {
  return d->mapDataProvider->getAvailableMaps();
}

void MapService::setCurrentMap(const QString& mapName) {
  auto metadata = d->mapDataProvider->loadMapMetadata(mapName);
  if (!metadata.has_value()) {
		qWarning() << "MapService: failed to load metadata for map:" << mapName;
		return;
  }

  d->currentMapName = mapName;
  d->currentMetadata = metadata;
  emit currentMapChanged(mapName);
}

std::optional<QString> MapService::getCurrentMap() const {
  return d->currentMapName;
}

std::optional<MapChunkData> MapService::loadChunk(int chunkX, int chunkZ) const {
  const auto mapName = d->currentMapName;
  if (!mapName.has_value()) {
		qWarning() << "MapService: no current map selected";
		return std::nullopt;
  }
  return d->mapDataProvider->loadChunk(*mapName, chunkX, chunkZ);
}

bool MapService::saveChunk(int chunkX, int chunkZ, const MapChunkData& chunkData) {
  const auto mapName = d->currentMapName;
  if (!mapName.has_value()) {
		qWarning() << "MapService: no current map selected";
		return false;
  }
  return d->mapDataProvider->saveChunk(*mapName, chunkX, chunkZ, chunkData);
}

bool MapService::chunkExists(int chunkX, int chunkZ) const {
  const auto mapName = d->currentMapName;
  if (!mapName.has_value()) {
		return false;
  }
  return d->mapDataProvider->chunkExists(*mapName, chunkX, chunkZ);
}

QList<QPoint> MapService::getChunkCoords() const {
  const auto mapName = d->currentMapName;
  if (!mapName.has_value()) {
		return {};
  }
  return d->mapDataProvider->getChunkCoords(*mapName);
}
