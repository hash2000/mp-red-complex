#pragma once
#include "DataLayer/users/i_users_data_provider.h"
#include <memory>

class Resources;

class UsersDataProviderJsonImpl : public IUsersDataProvider {
public:
	UsersDataProviderJsonImpl(Resources* resources);
	~UsersDataProviderJsonImpl() override;

	bool loadAllUsers(QHash<QString, UserData>& users) const override;
	std::optional<UserData> loadUser(const QString& userId) const override;
	bool saveUser(const UserData& user) override;
	bool deleteUser(const QString& userId) override;
	std::optional<UserData> findUserByLogin(const QString& login) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
