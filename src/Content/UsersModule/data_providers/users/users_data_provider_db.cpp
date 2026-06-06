#include "Content/UsersModule/data_providers/users/users_data_provider_db.h"
#include "Content/UsersModule/models/user/user.h"
#include "Resources/resources.h"
#include <QUuid>

class UsersDataProviderDb::Private {
public:
	Private(UsersDataProviderDb* parent)
		: q(parent) {}

	UsersDataProviderDb* q;
	Resources* resources;
};

UsersDataProviderDb::UsersDataProviderDb(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

UsersDataProviderDb::~UsersDataProviderDb() = default;

std::optional<UserData> UsersDataProviderDb::loadUser(const QString& loginHash) const {
	return std::nullopt;
}

bool UsersDataProviderDb::saveUser(const UserData& user) {
	return true;
}

bool UsersDataProviderDb::deleteUser(const QString& loginHash) {
	return true;
}
