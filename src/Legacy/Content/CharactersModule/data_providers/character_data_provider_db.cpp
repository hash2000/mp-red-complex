#include "Content/CharactersModule/data_providers/character_data_provider_db.h"
#include "Content/CharactersModule/data_providers/readers/character_reader.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/CharactersModule/models/character.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"

namespace {
static QString kSql_charactersUpdatePattern = R"(
		insert into user_characters (id, user_id, equipment_id, icon_path, level, name)
		select * from (
				values 
					%1
		) as t(id, user_id, equipment_id, icon_path, level, name)
		on conflict(id)
		do update set
				user_id = user_id,
				equipment_id = equipment_id,
				icon_path = icon_path,
				char_level = level,
				name = name,
				updated_at = datetime('now'),
				is_active = 1;
)";
static QString kSql_characterSelect = R"(
		select * from user_characters where is_active = 1
)";
static QString kSql_characterDelete = R"(
		update user_characters set is_active = 0
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

std::list<std::shared_ptr<Character>> CharacterDataProviderDb::userCharacters(const QString& userId) const {
	std::list<std::shared_ptr<Character>> result;
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return result;
	}

	auto reader = conn->executeQuery(QString("%1 and user_id = :id")
		.arg(kSql_characterSelect));
	if (!reader) {
		return result;
	}

	reader->bindValue(":id", userId);

	while (reader->next()) {
		Characters::Readers::CharacterReader rd;
		auto character = rd.read(*reader);
		result.push_back(character);
	}

	return result;
}

std::shared_ptr<Character> CharacterDataProviderDb::character(const QUuid& id) const {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return std::shared_ptr<Character>();
	}

	auto reader = conn->executeQuery(QString("%1 and id = :id")
		.arg(kSql_characterSelect));
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

	Characters::Readers::CharacterReader rd;
	auto character = rd.read(*reader);

	return std::move(character);
}

bool CharacterDataProviderDb::saveCharacters(const QString& userId, const std::list<std::shared_ptr<Character>>& characters) {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return false;
	}

	QStringList items;
	for (const auto& chr : characters) {
		items << QString("('%1', '%2', '%3', '%4', %5, '%6', datetime('now'))")
			.arg(chr->id.toString(QUuid::WithoutBraces))
			.arg(userId)
			.arg(chr->equipmentId.toString(QUuid::WithoutBraces))
			.arg(chr->iconPath)
			.arg(chr->level)
			.arg(chr->name);
	}
	QString valuesExpr = items.join(",");
	QString query = QString(kSql_charactersUpdatePattern)
		.arg(valuesExpr);
	
	return conn->execute(query);
}

bool CharacterDataProviderDb::deleteCharacter(const QUuid& id) {
	auto conn = d->databasesService->connection("game");
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
