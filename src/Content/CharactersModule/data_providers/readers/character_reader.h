#pragma once
#include <QString>
#include <memory>

class SQLiteReader;
class Character;

namespace Characters::Readers {
class CharacterReader {
public:
	CharacterReader(std::shared_ptr<Character> entry = std::shared_ptr<Character>());
	std::shared_ptr<Character> read(SQLiteReader& reader);

private:
	std::shared_ptr<Character> _entry;
};
}
