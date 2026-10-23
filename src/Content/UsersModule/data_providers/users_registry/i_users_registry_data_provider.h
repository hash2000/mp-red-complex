#pragma once
#include <QString>
#include <set>
#include <memory>

class IUsersRegistryDataProvider {
public:
	virtual ~IUsersRegistryDataProvider() = default;

	virtual bool addUser(const QString& userHash, const QString& username) = 0;
	virtual QString getUserHashByUsername(const QString& username) const = 0;
	virtual bool updateLastLogin(const QString& userHash) = 0;
	virtual bool removeUser(const QString& userHash) = 0;
	virtual QStringList listUsernames() const = 0;
};
