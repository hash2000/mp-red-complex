#pragma once
#include "DataLayer/maps/i_map_data_provider.h"
#include <QObject>
#include <QPixmap>
#include <memory>
#include <optional>

class TilesService;
class TexturesService;

class MapService : public QObject {
    Q_OBJECT
public:
    explicit MapService(
        TilesService* tilesService,
        TexturesService* texturesService,
        IMapDataProvider* mapDataProvider,
        QObject* parent = nullptr);
    ~MapService() override;

    // Загрузить метаданные карты
    std::optional<MapMetadata> loadMapMetadata(const QString& mapName) const;

    // Сохранить метаданные карты
    bool saveMapMetadata(const QString& mapName, const MapMetadata& metadata);

    // Удалить карту
    bool deleteMap(const QString& mapName);

    // Получить список всех доступных карт
    QList<QString> getAvailableMaps() const;

    // Получить QPixmap тайловой карты через TilesService
    std::optional<QPixmap> getTilemapPixmap(const QString& mapName) const;

    // Установить текущую активную карту
    void setCurrentMap(const QString& mapName);

    // Получить текущую активную карту
    std::optional<QString> getCurrentMap() const;

    // Загрузить данные чанка для текущей карты
    std::optional<MapChunkData> loadChunk(int chunkX, int chunkZ) const;

    // Сохранить данные чанка для текущей карты
    bool saveChunk(int chunkX, int chunkZ, const MapChunkData& chunkData);

    // Проверить существование чанка
    bool chunkExists(int chunkX, int chunkZ) const;

    // Получить список сохранённых чанков
    QList<QPoint> getChunkCoords() const;

signals:
    void currentMapChanged(const QString& mapName);

private:
    class Private;
    std::unique_ptr<Private> d;
};
