#include "Content/CharactersModule/data_providers/readers/character_reader.h"
#include "Content/CharactersModule/models/character.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"
#include <QDebug>

namespace Characters::Readers {
CharacterReader::CharacterReader(std::shared_ptr<Character> entry) : _entry(entry) {
	if (!_entry) {
		_entry = std::make_shared<Character>();
	}
}

std::shared_ptr<Character> CharacterReader::read(SQLiteReader& reader) {
	_entry->id = QUuid::fromString(reader.value("id").toString());
	_entry->userId = reader.value("user_id").toString();
	_entry->name = reader.value("name").toString();
	_entry->equipmentId = QUuid::fromString(reader.value("equipment_id").toString());
	_entry->iconPath = reader.value("icon_path").toString();
	_entry->level = reader.value("char_level").toInt();
	return _entry;
}
}
