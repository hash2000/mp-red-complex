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

    // Сериализовать MapMetadata в JSON
    static QJsonObject metadataToJson(const MapMetadata& metadata) {
        QJsonObject obj;
        obj["id"] = metadata.id.toString();
        obj["name"] = metadata.name;
        obj["tileTexturePath"] = metadata.tileTexturePath;

        QJsonObject mapSize;
        mapSize["width"] = metadata.mapSize.width();
        mapSize["height"] = metadata.mapSize.height();
        obj["mapSize"] = mapSize;

        QJsonObject tileSize;
        tileSize["width"] = metadata.tileSize.width();
        tileSize["height"] = metadata.tileSize.height();
        obj["tileSize"] = tileSize;

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
        metadata.tileTexturePath = obj["tileTexturePath"].toString();

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

        return metadata;
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

    QDir dir(dataDir);
    const QString mapDir = QString("assets/maps/%1").arg(mapName);
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
