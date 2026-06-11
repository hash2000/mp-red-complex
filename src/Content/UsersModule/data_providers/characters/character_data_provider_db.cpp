#include "Content/UsersModule/data_providers/characters/character_data_provider_db.h"
#include "Content/UsersModule/data_providers/migrations/users_migrations.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/UsersModule/models/character/character.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"

class CharacterDataProviderDb::Private {
public:
	Private(CharacterDataProviderDb* parent) : q(parent) {}
	CharacterDataProviderDb* q;
	DatabasesService* databasesService;
};


CharacterDataProviderDb::CharacterDataProviderDb(DatabasesService* databasesService)
: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("users");
	UsersMigrations::build(migrator);
}

CharacterDataProviderDb::~CharacterDataProviderDb() = default;

std::optional<Character> CharacterDataProviderDb::loadCharacter(const QUuid& id) const {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return std::nullopt;
	}

	auto reader = conn->executeQuery(R"(
			SELECT * FROM characters c WHERE c.id = :id and is_active = 1
	)");

	reader->bindValue(":id", id.toString(QUuid::WithoutBraces));

	if (!reader || !reader->next()) {
		qWarning() << "Can't find character:" << id << "Maybe is inactive";
		return std::nullopt;
	}

	Character character;
	character.id = id;
	character.name = reader->value("name").toString();
	character.equipmentId = QUuid::fromString(reader->value("equipment_id").toString());
	character.iconPath = reader->value("icon_path").toString();
	character.level = reader->value("char_level").toInt();

	return character;
}

bool CharacterDataProviderDb::saveCharacter(const Character& character) {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return false;
	}

	auto insert = conn->prepare(R"(
		INSERT INTO characters(id, name, equipment_id, icon_path, char_level)
		VALUES(:id, :name, :equipment_id, :icon_path, :char_level);
	)");

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

	auto reader = conn->prepare(R"(
			UPDATE characters SET is_active = 0
			WHERE id = :id;
	)");

	reader->bindValue(":id", id);

	if (!reader || !reader->exec()) {
		qWarning() << "Can't delete characters:" << id;
		return false;
	}

	return true;
}
