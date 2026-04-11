#pragma once
#include <QString>
#include <QSize>
#include <QUuid>
#include <optional>

// Метаданные карты
struct MapMetadata {
    QUuid id;                          // Уникальный ID карты
    QString name;                      // Название карты
    QString tileTexturePath;           // Путь к тайловой текстуре (для TilesService::getTilemap)
    QSize mapSize = { 100, 100 };      // Размер карты в тайлах
    QSize tileSize = { 32, 32 };       // Размер одного тайла в пикселях
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
};
