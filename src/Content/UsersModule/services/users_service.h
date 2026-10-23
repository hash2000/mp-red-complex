#pragma once
#include <QObject>
#include <memory>
#include <optional>
#include <map>
#include <list>

class ImagesService;
class IUsersRegistryDataProvider;
class UserView;
class Resources;
class DatabasesService;

/// Сервис управления пользователями
class UsersService : public QObject {
	Q_OBJECT
public:
	explicit UsersService(
		Resources* resources,
		DatabasesService* databasesService,
		IUsersRegistryDataProvider* usersRegistryDataProvider,
		ImagesService* imagesService,
		QObject* parent = nullptr);

	~UsersService() override;

	bool login(
		const QString& username,
		const QString& password);

	void logout();

	bool isAuthenticated() const;

	QString currentUserHash();
	QByteArray currentUserEncryptedSeed();

signals:
	void loginSuccess(const UserView& user);
	void loggedOut();

private:
	class Private;
	std::unique_ptr<Private> d;
};
