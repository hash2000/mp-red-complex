#include "Content/UsersModule/data_providers/users/users_data_provider_db.h"
#include "Content/UsersModule/services/users_service.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/UsersModule/models/user/user.h"
#include <QUuid>

class UsersDataProviderDb::Private {
public:
	Private(UsersDataProviderDb* parent)
		: q(parent) {}

	UsersDataProviderDb* q;
	DatabasesService* databasesService;
};

UsersDataProviderDb::UsersDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
}

UsersDataProviderDb::~UsersDataProviderDb() = default;

std::optional<UserData> UsersDataProviderDb::loadUser(const QString& loginHash) const {
	auto connection = d->databasesService->connection("users");
	if (!connection) {
		return std::nullopt;
	}


	return std::nullopt;
}

bool UsersDataProviderDb::saveUser(const UserData& user) {
	return true;
}

bool UsersDataProviderDb::deleteUser(const QString& loginHash) {
	return true;
}
