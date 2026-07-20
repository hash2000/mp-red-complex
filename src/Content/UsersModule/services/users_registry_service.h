#pragma once
#include <QObject>
#include <memory>

class Resources;
class IRegisterUserDataProvider;
class CurrentUserService;

class UsersRegistryService : public QObject {
	Q_OBJECT
public:
	explicit UsersRegistryService(
		Resources* resources,
		IRegisterUserDataProvider* registerUserDataProvider,
		QObject* parent = nullptr);

	~UsersRegistryService() override;

	bool registerUser(const QString& username, const QString& password, QStringList& outSeedPhrase);

private:
	class Private;
	std::unique_ptr<Private> d;
};
