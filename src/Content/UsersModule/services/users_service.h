#pragma once
#include <QObject>
#include <memory>
#include <optional>
#include <map>
#include <list>

class ImagesService;
class IUsersDataProvider;
class UserView;
class CurrentUserContext;
class Resources;

/// Сервис управления пользователями
class UsersService : public QObject {
	Q_OBJECT
public:
	explicit UsersService(
		Resources* resources,
		IUsersDataProvider* usersDataProvider,
		ImagesService* imagesService,
		QObject* parent = nullptr);

	~UsersService() override;

	std::optional<QString> login(
		const QString& login,
		const QString& password);

	void logout();

	bool isAuthenticated() const;

	CurrentUserContext* currentUserContext();

signals:
	void loginSuccess(const UserView& user);
	void loggedOut();

private:
	class Private;
	std::unique_ptr<Private> d;
};
