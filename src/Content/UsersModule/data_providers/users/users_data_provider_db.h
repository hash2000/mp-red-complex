#pragma once
#include "Content/UsersModule/data_providers/users/i_users_data_provider.h"
#include <QString>
#include <memory>

class DatabasesService;

class UsersDataProviderDb : public IUsersDataProvider {
public:
	UsersDataProviderDb(DatabasesService* databasesService);
	~UsersDataProviderDb() override;

	std::shared_ptr<UserData> loadUser(const QString& loginHash) const override;
	bool saveUser(const std::shared_ptr<UserData> user) override;
	bool deleteUser(const QString& loginHash) override;
	bool containsUser(const QString& loginHash) override;
	std::set<QString> permissions(const QString& id) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
