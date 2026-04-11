#include "DataLayer/maps/map_data_provider_json_impl.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <qDebug>

class MapDataProviderJsonImpl::Private {
public:
    Private(MapDataProviderJsonImpl* parent) : q(parent) {}
    MapDataProviderJsonImpl* q;
    Resources* resources = nullptr;

    // Получить путь к файлу metadata.json для карты
    // mapName — это имя карты, например "test_map"
    // Файл хранится в assets/maps/{map_name}/metadata.json
    static QString buildMetadataFilePath(const QString& mapName) {
        return QString("maps/%1/metadata.json").arg(mapName);
    }

    // Получить путь к файлу чанка
    // Файл хранится в assets/maps/{map_name}/chunks/{chunkX}_{chunkZ}.json
    static QString buildChunkFilePath(const QString& mapName, int chunkX, int chunkZ) {
        return QString("maps/%1/chunks/%2_%3.json").arg(mapName).arg(chunkX).arg(chunkZ);
    }

    // Сериализовать MapMetadata в JSON
    static QJsonObject metadataToJson(const MapMetadata& metadata) {
        QJsonObject obj;
        obj["id"] = metadata.id.toString();
        obj["name"] = metadata.name;

        QJsonObject mapSize;
        mapSize["width"] = metadata.mapSize.width();
        mapSize["height"] = metadata.mapSize.height();
        obj["mapSize"] = mapSize;

        QJsonObject tileSize;
        tileSize["width"] = metadata.tileSize.width();
        tileSize["height"] = metadata.tileSize.height();
        obj["tileSize"] = tileSize;

        QJsonObject chunkSize;
        chunkSize["width"] = metadata.chunkSize.width();
        chunkSize["height"] = metadata.chunkSize.height();
        obj["chunkSize"] = chunkSize;

        return obj;
    }

    // Десериализовать MapMetadata из JSON
    static MapMetadata metadataFromJson(const QJsonObject& obj) {
        MapMetadata metadata;
        metadata.id = QUuid::fromString(obj["id"].toString());
        if (metadata.id.isNull()) {
            metadata.id = QUuid::createUuid();
        }
        metadata.name = obj["name"].toString();

        if (obj.contains("mapSize") && obj["mapSize"].isObject()) {
            const QJsonObject sizeObj = obj["mapSize"].toObject();
            metadata.mapSize.setWidth(sizeObj["width"].toInt(100));
            metadata.mapSize.setHeight(sizeObj["height"].toInt(100));
        }

        if (obj.contains("tileSize") && obj["tileSize"].isObject()) {
            const QJsonObject sizeObj = obj["tileSize"].toObject();
            metadata.tileSize.setWidth(sizeObj["width"].toInt(32));
            metadata.tileSize.setHeight(sizeObj["height"].toInt(32));
        }

        if (obj.contains("chunkSize") && obj["chunkSize"].isObject()) {
            const QJsonObject sizeObj = obj["chunkSize"].toObject();
            metadata.chunkSize.setWidth(sizeObj["width"].toInt(32));
            metadata.chunkSize.setHeight(sizeObj["height"].toInt(32));
        }

        return metadata;
    }

    // Сериализовать MapChunkData в JSON
    static QJsonObject chunkDataToJson(const MapChunkData& chunkData) {
        QJsonObject obj;
        obj["atlasName"] = chunkData.atlasName;

        QJsonObject chunkSize;
        chunkSize["width"] = chunkData.chunkSize.width();
        chunkSize["height"] = chunkData.chunkSize.height();
        obj["chunkSize"] = chunkSize;

        // Сериализуем tileGrid как массив массивов (2D сетка)
        QJsonArray gridArray;
        const int width = chunkData.chunkSize.width();
        const int height = chunkData.chunkSize.height();
        for (int z = 0; z < height; ++z) {
            QJsonArray row;
            for (int x = 0; x < width; ++x) {
                const int index = z * width + x;
                row.append(chunkData.tileGrid.value(index, -1));
            }
            gridArray.append(row);
        }
        obj["tileGrid"] = gridArray;

        return obj;
    }

    // Десериализовать MapChunkData из JSON
    static MapChunkData chunkDataFromJson(const QJsonObject& obj) {
        MapChunkData chunkData;
        chunkData.atlasName = obj["atlasName"].toString();

        if (obj.contains("chunkSize") && obj["chunkSize"].isObject()) {
            const QJsonObject sizeObj = obj["chunkSize"].toObject();
            chunkData.chunkSize.setWidth(sizeObj["width"].toInt(32));
            chunkData.chunkSize.setHeight(sizeObj["height"].toInt(32));
        }

        // Десериализуем tileGrid
        const int width = chunkData.chunkSize.width();
        const int height = chunkData.chunkSize.height();
        chunkData.tileGrid.resize(width * height, -1);

        if (obj.contains("tileGrid") && obj["tileGrid"].isArray()) {
            const QJsonArray gridArray = obj["tileGrid"].toArray();
            for (int z = 0; z < qMin(height, static_cast<int>(gridArray.size())); ++z) {
                if (gridArray[z].isArray()) {
                    const QJsonArray row = gridArray[z].toArray();
                    for (int x = 0; x < qMin(width, static_cast<int>(row.size())); ++x) {
                        const int index = z * width + x;
                        chunkData.tileGrid[index] = row[x].toInt(-1);
                    }
                }
            }
        }

        return chunkData;
    }
};

MapDataProviderJsonImpl::MapDataProviderJsonImpl(Resources* resources)
    : d(std::make_unique<Private>(this)) {
    d->resources = resources;
}

MapDataProviderJsonImpl::~MapDataProviderJsonImpl() = default;

std::optional<MapMetadata> MapDataProviderJsonImpl::loadMapMetadata(const QString& mapName) const {
    const QString metadataFile = Private::buildMetadataFilePath(mapName);

    QJsonObject json;
    Format::Json::DataReader reader(d->resources, "assets", metadataFile);
    if (!reader.read(json)) {
        return std::nullopt;
    }

    return Private::metadataFromJson(json);
}

bool MapDataProviderJsonImpl::saveMapMetadata(const QString& mapName, const MapMetadata& metadata) {
    const QString metadataFile = Private::buildMetadataFilePath(mapName);

    QJsonObject root = Private::metadataToJson(metadata);
    QJsonDocument doc(root);

    const auto dataDir = d->resources->Variables.get("Resources.Path", "")
        .toString();
    if (dataDir.isEmpty()) {
        qWarning() << "MapDataProviderJsonImpl: Resources.Path not found in variables";
        return false;
    }

    QDir dir(dataDir + "/assets");
    const QString mapDir = QString("maps/%1").arg(mapName);
    if (!dir.exists(mapDir)) {
        dir.mkpath(mapDir);
    }

    QFile file(dir.filePath(metadataFile));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "MapDataProviderJsonImpl: can't open file for writing:" << file.fileName();
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

bool MapDataProviderJsonImpl::deleteMap(const QString& mapName) {
    const auto dataDir = d->resources->Variables.get("Resources.Path", "")
        .toString();
    if (dataDir.isEmpty()) {
        qWarning() << "MapDataProviderJsonImpl: Resources.Path not found in variables";
        return false;
    }

    QDir dir(dataDir);
    const QString mapDir = QString("assets/maps/%1").arg(mapName);
    
    if (!dir.exists(mapDir)) {
        return false;
    }

    return dir.rmdir(mapDir);
}

QList<QString> MapDataProviderJsonImpl::getAvailableMaps() const {
    QList<QString> maps;

    const auto dataDir = d->resources->Variables.get("Resources.Path", "")
        .toString();
    if (dataDir.isEmpty()) {
        qWarning() << "MapDataProviderJsonImpl: Resources.Path not found in variables";
        return maps;
    }

    QDir dir(dataDir + "/assets/maps");
    if (!dir.exists()) {
        return maps;
    }

    // Получаем все поддиректории (каждая — это карта)
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        // Проверяем, есть ли metadata.json
        const QString metadataPath = entry.absoluteFilePath() + "/metadata.json";
        if (QFile::exists(metadataPath)) {
            maps.append(entry.fileName());
        }
    }

    return maps;
}

std::optional<MapChunkData> MapDataProviderJsonImpl::loadChunk(const QString& mapName, int chunkX, int chunkZ) const {
    const QString chunkFile = Private::buildChunkFilePath(mapName, chunkX, chunkZ);

    QJsonObject json;
    Format::Json::DataReader reader(d->resources, "assets", chunkFile);
    if (!reader.read(json)) {
        return std::nullopt;
    }

    return Private::chunkDataFromJson(json);
}

bool MapDataProviderJsonImpl::saveChunk(const QString& mapName, int chunkX, int chunkZ, const MapChunkData& chunkData) {
    const QString chunkFile = Private::buildChunkFilePath(mapName, chunkX, chunkZ);

    QJsonObject root = Private::chunkDataToJson(chunkData);
    QJsonDocument doc(root);

    const auto dataDir = d->resources->Variables.get("Resources.Path", "")
        .toString();
    if (dataDir.isEmpty()) {
        qWarning() << "MapDataProviderJsonImpl: Resources.Path not found in variables";
        return false;
    }

    QDir dir(dataDir);
    const QString chunkDir = QString("assets/maps/%1/chunks").arg(mapName);
    if (!dir.exists(chunkDir)) {
        dir.mkpath(chunkDir);
    }

    QFile file(dir.filePath(chunkFile));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "MapDataProviderJsonImpl: can't open chunk file for writing:" << file.fileName();
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

bool MapDataProviderJsonImpl::chunkExists(const QString& mapName, int chunkX, int chunkZ) const {
    const auto dataDir = d->resources->Variables.get("Resources.Path", "")
        .toString();
    if (dataDir.isEmpty()) {
        return false;
    }

    const QString chunkFile = Private::buildChunkFilePath(mapName, chunkX, chunkZ);
    return QFile::exists(dataDir + "/assets/" + chunkFile);
}

QList<QPoint> MapDataProviderJsonImpl::getChunkCoords(const QString& mapName) const {
    QList<QPoint> coords;

    const auto dataDir = d->resources->Variables.get("Resources.Path", "")
        .toString();
    if (dataDir.isEmpty()) {
        return coords;
    }

    QDir dir(dataDir + "/assets/maps/" + mapName + "/chunks");
    if (!dir.exists()) {
        return coords;
    }

    // Получаем все файлы *.json в директории chunks
    const QFileInfoList entries = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
    for (const QFileInfo& entry : entries) {
        // Имя файла: {chunkX}_{chunkZ}.json
        const QString baseName = entry.baseName();
        const QStringList parts = baseName.split('_');
        if (parts.size() == 2) {
            bool okX = false, okZ = false;
            const int x = parts[0].toInt(&okX);
            const int z = parts[1].toInt(&okZ);
            if (okX && okZ) {
                coords.append(QPoint(x, z));
            }
        }
    }

    return coords;
}
