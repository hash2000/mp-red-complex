#pragma once
#include "DataLayer/users/i_users_data_provider.h"
#include <memory>

class Resources;

class UsersDataProviderJsonImpl : public IUsersDataProvider {
public:
	UsersDataProviderJsonImpl(Resources* resources);
	~UsersDataProviderJsonImpl() override;

	std::optional<UserData> loadUser(const QString& loginHash) const override;
	bool saveUser(const UserData& loginHash) override;
	bool deleteUser(const QString& loginHash) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
