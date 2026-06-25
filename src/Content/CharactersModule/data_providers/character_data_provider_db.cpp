#include "Content/CharactersModule/data_providers/character_data_provider_db.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/CharactersModule/models/character.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"

namespace {
static QString kSql_characterSelect = R"(
		select * from characters c where c.id = :id and is_active = 1
)";
static QString kSql_characterInsert = R"(
		insert into characters(id, name, equipment_id, icon_path, char_level)
		values(:id, :name, :equipment_id, :icon_path, :char_level);
)";
static QString kSql_characterDelete = R"(
		update characters set is_active = 0
		where id = :id;
)";
}

class CharacterDataProviderDb::Private {
public:
	Private(CharacterDataProviderDb* parent) : q(parent) {}
	CharacterDataProviderDb* q;
	DatabasesService* databasesService;
};


CharacterDataProviderDb::CharacterDataProviderDb(DatabasesService* databasesService)
: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
}

CharacterDataProviderDb::~CharacterDataProviderDb() = default;

std::shared_ptr<Character> CharacterDataProviderDb::userCharacter(const QUuid& id) const {
	return std::shared_ptr<Character>();
}

std::shared_ptr<Character> CharacterDataProviderDb::character(const QUuid& id) const {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return std::shared_ptr<Character>();
	}

	auto reader = conn->executeQuery(kSql_characterSelect);
	if (!reader) {
		return std::shared_ptr<Character>();
	}

	reader->bindValue(":id", id
		.toString(QUuid::WithoutBraces)
		.toLower());

	if (!reader->next()) {
		qWarning() << "Can't find character:" << id << "Maybe is inactive";
		return std::shared_ptr<Character>();
	}

	auto character = std::make_shared<Character>();
	character->id = id;
	character->name = reader->value("name").toString();
	character->equipmentId = QUuid::fromString(reader->value("equipment_id").toString());
	character->iconPath = reader->value("icon_path").toString();
	character->level = reader->value("char_level").toInt();

	return std::move(character);
}

bool CharacterDataProviderDb::saveCharacter(const Character& character) {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return false;
	}

	auto insert = conn->prepare(kSql_characterInsert);
	insert->bindValues({
		{ ":id", character.id.toString(QUuid::WithoutBraces) },
		{ ":name", character.name },
		{ ":equipment_id", character.equipmentId.toString(QUuid::WithoutBraces) },
		{ ":icon_path", character.iconPath },
		{ ":char_level", character.level },
		});

	if (!insert->exec()) {
		qWarning() << "Save characters failed." << character.name;
		conn->rollback();
		return false;
	}

	return true;
}

bool CharacterDataProviderDb::deleteCharacter(const QUuid& id) {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return false;
	}

	auto reader = conn->prepare(kSql_characterDelete);
	reader->bindValue(":id", id);

	if (!reader || !reader->exec()) {
		qWarning() << "Can't delete characters:" << id;
		return false;
	}

	return true;
}
