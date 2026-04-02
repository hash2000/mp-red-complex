#include "DataLayer/characters/character_data_provider_json_impl.h"
#include "DataLayer/characters/character.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"

class CharacterDataProviderJsonImpl::Private {
public:
	Private(CharacterDataProviderJsonImpl* parent) : q(parent) {}

	CharacterDataProviderJsonImpl* q;
	Resources* resources;
};


CharacterDataProviderJsonImpl::CharacterDataProviderJsonImpl(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

CharacterDataProviderJsonImpl::~CharacterDataProviderJsonImpl() = default;

bool CharacterDataProviderJsonImpl::loadCharacter(const QUuid& id, Character& character) {
	const auto path = QString("characters/%1.json")
		.arg(id.toString(QUuid::StringFormat::WithoutBraces).toLower());

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "data", path);
	if (!reader.read(json)) {
		return false;
	}

	return true;
}
