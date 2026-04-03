#include "DataLayer/characters/character_data_provider_json_impl.h"
#include "DataLayer/characters/character.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"

class CharacterDataProviderJsonImpl::Private {
public:
	Private(CharacterDataProviderJsonImpl* parent) : q(parent) {}

	QString getCharacterFilePath(const QUuid& id) const {
		const auto path = QString("characters/%1.json")
			.arg(id.toString(QUuid::StringFormat::WithoutBraces).toLower());

		return path;
	}

	CharacterDataProviderJsonImpl* q;
	Resources* resources;
};


CharacterDataProviderJsonImpl::CharacterDataProviderJsonImpl(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

CharacterDataProviderJsonImpl::~CharacterDataProviderJsonImpl() = default;

bool CharacterDataProviderJsonImpl::loadCharacter(const QUuid& id, Character& character) const {
	const auto path = d->getCharacterFilePath(id);

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "data", path);
	if (!reader.read(json)) {
		return false;
	}

	character.id = id;
	character.equipmentId = QUuid::fromString(json["equipmentId"].toString());
	character.inventoryId = QUuid::fromString(json["inventoryId"].toString());
	character.iconPath = json["iconPath"].toString();

	return true;
}

bool CharacterDataProviderJsonImpl::saveCharacter(const Character& character) {
	const auto path = d->getCharacterFilePath(character.id);
	auto dataDir = d->resources->Variables.get("Resources.Path", "")
		.toString();
	QDir dir(dataDir);
	if (!dir.exists(path)) {
		dir.mkpath(path);
	}

	QJsonObject json;
	json["id"] = character.id.toString(QUuid::StringFormat::WithoutBraces);
	json["equipmentId"] = character.equipmentId.toString(QUuid::StringFormat::WithoutBraces);
	json["inventoryId"] = character.inventoryId.toString(QUuid::StringFormat::WithoutBraces);
	json["iconPath"] = character.iconPath;

	QJsonDocument doc(json);

	const auto filePath = dir.filePath("data/" + path);
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	file.write(doc.toJson());
	return true;
}

bool CharacterDataProviderJsonImpl::deleteCharacter(const QUuid& id) {
	const auto path = d->getCharacterFilePath(id);
	return QFile::remove(path);
}
