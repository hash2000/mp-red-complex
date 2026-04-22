#include "DataLayer/images/tile_groups_data_provider_json_impl.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <qDebug>

class TileGroupsDataProviderJsonImpl::Private {
public:
	Private(TileGroupsDataProviderJsonImpl* parent) : q(parent) {}
	TileGroupsDataProviderJsonImpl* q;
	Resources* resources = nullptr;

	// Получить имя файла групп для текстуры
	// texturePath — это путь относительно assets/textures/tiles/, например "ground.png"
	// groups файл хранится в data/textures/tiles/{name}.json
	static QString buildGroupsFilePath(const QString& texturePath) {
		const QString textureName = QFileInfo(texturePath).baseName();
		return QString("textures/tiles/%1.json").arg(textureName);
	}

	// Сериализовать TileGroup в JSON
	static QJsonObject groupToJson(const TileGroup& group) {
		QJsonObject obj;
		obj["id"] = group.id.toString();
		obj["name"] = group.name;

		QJsonArray tileIdsArray;
		for (int tileId : group.tileIds) {
			tileIdsArray.append(tileId);
		}
		obj["tileIds"] = tileIdsArray;

		return obj;
	}

	// Десериализовать TileGroup из JSON
	static TileGroup groupFromJson(const QJsonObject& obj) {
		TileGroup group;
		group.id = QUuid::fromString(obj["id"].toString());
		if (group.id.isNull()) {
			group.id = QUuid::createUuid();
		}
		group.name = obj["name"].toString();

		const QJsonArray tileIdsArray = obj["tileIds"].toArray();
		for (const QJsonValue& val : tileIdsArray) {
			group.tileIds.append(val.toInt());
		}

		return group;
	}

	static void addMetadata(QJsonObject& obj, const TileSetMetadata& metadata) {
		QJsonObject size;
		size["x"] = metadata.gridSize.x;
		size["y"] = metadata.gridSize.y;

		QJsonObject grid;
		grid["size"] = size;
		obj["grid"] = grid;
	}
};

TileGroupsDataProviderJsonImpl::TileGroupsDataProviderJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

TileGroupsDataProviderJsonImpl::~TileGroupsDataProviderJsonImpl() = default;

QList<TileGroup> TileGroupsDataProviderJsonImpl::loadGroups(const QString& texturePath) const {
	QList<TileGroup> groups;

	const QString groupsFile = Private::buildGroupsFilePath(texturePath);

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "assets", groupsFile);
	if (!reader.read(json)) {
		return groups; // Пустой список, файл не найден или невалидный
	}

	// Ожидаем JSON-массив групп
	if (!json.contains("groups") || !json["groups"].isArray()) {
		return groups;
	}

	const QJsonArray array = json["groups"].toArray();
	for (const QJsonValue& val : array) {
		if (val.isObject()) {
			groups.append(Private::groupFromJson(val.toObject()));
		}
	}

	return groups;
}

bool TileGroupsDataProviderJsonImpl::saveGroup(const QString& texturePath, const TileGroup& group, const TileSetMetadata& metadata) {
	const QString groupsFile = Private::buildGroupsFilePath(texturePath);

	// Загружаем существующие группы
	QList<TileGroup> existingGroups = loadGroups(texturePath);

	// Ищем группу с таким же ID для обновления
	bool found = false;
	for (int i = 0; i < existingGroups.size(); ++i) {
		if (existingGroups[i].id == group.id) {
			existingGroups[i] = group;
			found = true;
			break;
		}
	}

	// Если не нашли, добавляем новую
	if (!found) {
		existingGroups.append(group);
	}

	// Сериализуем в JSON
	QJsonArray array;
	for (const TileGroup& g : existingGroups) {
		array.append(Private::groupToJson(g));
	}

	QJsonObject root;
	Private::addMetadata(root, metadata);
	root["groups"] = array;
	QJsonDocument doc(root);

	auto dataDir = d->resources->Variables.get("Resources.Path", "")
		.toString();
	if (dataDir.isEmpty()) {
		qWarning() << "TileGroupsDataProviderJsonImpl: Resources.Path not found in variables";
		return false;
	}

	QDir dir(dataDir);
	if (!dir.exists("assets/textures/tiles")) {
		dir.mkpath("assets/textures/tiles");
	}

	QFile file(dir.filePath("assets/" + groupsFile));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "TileGroupsDataProviderJsonImpl: can't open file for writing:" << file.fileName();
		return false;
	}

	file.write(doc.toJson());
	return true;
}

bool TileGroupsDataProviderJsonImpl::deleteGroup(const QUuid& groupId, const TileSetMetadata& metadata) {
	const auto dataDir = d->resources->Variables.get("Resources.Path", "")
		.toString();
	if (dataDir.isEmpty()) {
		qWarning() << "TileGroupsDataProviderJsonImpl: Resources.Path not found in variables";
		return false;
	}

	const auto filePath = dataDir + "/assets/textures/tiles/" + metadata.fileName + ".json";

	// Пробуем загрузить группы из этого файла
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	const QByteArray data = file.readAll();
	file.close();

	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(data, &error);
	if (error.error != QJsonParseError::NoError || !doc.isObject()) {
		return false;
	}

	const QJsonObject root = doc.object();
	if (!root.contains("groups") || !root["groups"].isArray()) {
		return false;
	}

	// Ищем группу с таким ID
	QJsonArray groupsArray = root["groups"].toArray();
	bool found = false;
	for (int i = 0; i < groupsArray.size(); ++i) {
		const QJsonObject groupObj = groupsArray[i].toObject();
		const QString existingId = groupObj["id"].toString();
		if (existingId == groupId.toString()) {
			groupsArray.removeAt(i);
			found = true;
			break;
		}
	}

	if (found) {
		// Сохраняем обновлённый файл
		QJsonObject updatedRoot;
		Private::addMetadata(updatedRoot, metadata);
		updatedRoot["groups"] = groupsArray;
		QJsonDocument updatedDoc(updatedRoot);

		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			qWarning() << "TileGroupsDataProviderJsonImpl: can't open file for writing:" << file.fileName();
			return false;
		}

		file.write(updatedDoc.toJson());
		file.close();
		return true;
	}

	return false;
}

bool TileGroupsDataProviderJsonImpl::deleteGroupsForTexture(const QString& texturePath) {
	const QString groupsFile = Private::buildGroupsFilePath(texturePath);

	auto dataDir = d->resources->Variables.get("Resources.Path", "")
		.toString();
	if (dataDir.isEmpty()) {
		qWarning() << "TileGroupsDataProviderJsonImpl: Resources.Path not found in variables";
		return false;
	}

	QDir dir(dataDir);
	const QString fullPath = dir.filePath("assets/" + groupsFile);
	return QFile::remove(fullPath);
}

std::optional<TileSetMetadata> TileGroupsDataProviderJsonImpl::loadTileSetMetadata(const QString& path) const {
	// Формируем путь к JSON файлу метаданных: {name}.json вместо {name}.png
	const QString baseName = QFileInfo(path).baseName();
	const QString jsonPath = Private::buildGroupsFilePath(baseName);

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "assets", jsonPath);
	if (!reader.read(json)) {
		return std::nullopt;
	}

	TileSetMetadata metadata;

	// Читаем grid.size.x и grid.size.y
	if (json.contains("grid") && json["grid"].isObject()) {
		const QJsonObject gridObj = json["grid"].toObject();
		if (gridObj.contains("size") && gridObj["size"].isObject()) {
			const QJsonObject sizeObj = gridObj["size"].toObject();
			if (sizeObj.contains("x")) {
				metadata.gridSize.x = sizeObj["x"].toInt();
			}
			if (sizeObj.contains("y")) {
				metadata.gridSize.y = sizeObj["y"].toInt();
			}
		}
	}

	metadata.fileName = baseName;

	return metadata;
}
