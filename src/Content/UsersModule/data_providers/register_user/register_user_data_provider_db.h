#pragma once
#include "Content/UsersModule/data_providers/register_user/i_register_user_data_provider.h"
#include <QString>
#include <memory>

class DatabasesService;

class RegisterUserDataProviderDb : public IRegisterUserDataProvider {
public:
	RegisterUserDataProviderDb(DatabasesService* databasesService);
	~RegisterUserDataProviderDb() override;

	bool addUser(const QString& userHash, const QString& username) override;
	QString getUserHashByUsername(const QString& username) const override;
	bool updateLastLogin(const QString& userHash) override;
	bool removeUser(const QString& userHash) override;
	QStringList listUsernames() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
