#pragma once
#include <QObject>
#include <memory>

class Resources;
class IUsersRegistryDataProvider;
class CurrentUserService;

class UsersRegistryService : public QObject {
	Q_OBJECT
public:
	explicit UsersRegistryService(
		Resources* resources,
		IUsersRegistryDataProvider* usersRegistryDataProvider,
		QObject* parent = nullptr);

	~UsersRegistryService() override;

	bool registerUser(const QString& username, const QString& password, QStringList& outSeedPhrase);
	bool restoreFromSeed(const QStringList& seedPhrase, const QString& newPassword);

private:
	class Private;
	std::unique_ptr<Private> d;
};
