#pragma once
#include <QString>
#include <QSize>
#include <QPoint>
#include <QUuid>
#include <optional>
#include <QVector>

// Метаданные карты
struct MapMetadata {
    QUuid id;                          // Уникальный ID карты
    QString name;                      // Название карты (дублируется из пути)
    int tileSize = 32;                 // Размер одного тайла в пикселях
    QSize chunkSize = { 32, 32 };      // Размер чанка в тайлах
};

// Данные чанка карты
struct MapChunkData {
    QString atlasName;                 // Имя атласа для этого чанка
    QSize chunkSize = { 32, 32 };      // Размер чанка в тайлах
    QVector<int> tileGrid;             // Сетка тайлов (chunkSize.width * chunkSize.height)
                                       // Индекс: z * chunkSize.width + x
                                       // -1 = пустой тайл
};

// Интерфейс провайдера данных для карт
class IMapDataProvider {
public:
    virtual ~IMapDataProvider() = default;

    // Загрузить метаданные карты
    virtual std::optional<MapMetadata> loadMapMetadata(const QString& mapName) const = 0;

    // Сохранить метаданные карты
    virtual bool saveMapMetadata(const QString& mapName, const MapMetadata& metadata) = 0;

    // Удалить карту
    virtual bool deleteMap(const QString& mapName) = 0;

    // Получить список всех доступных карт
    virtual QList<QString> getAvailableMaps() const = 0;

    // Загрузить данные чанка по координатам
    virtual std::optional<MapChunkData> loadChunk(const QString& mapName, int chunkX, int chunkZ) const = 0;

    // Сохранить данные чанка
    virtual bool saveChunk(const QString& mapName, int chunkX, int chunkZ, const MapChunkData& chunkData) = 0;

    // Проверить существование чанка
    virtual bool chunkExists(const QString& mapName, int chunkX, int chunkZ) const = 0;

    // Получить список сохранённых чанков для карты
    virtual QList<QPoint> getChunkCoords(const QString& mapName) const = 0;
};
