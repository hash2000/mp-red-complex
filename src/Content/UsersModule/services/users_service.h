#pragma once
#include <QObject>
#include <memory>
#include <optional>
#include <map>
#include <list>

class ImagesService;
class IUsersDataProvider;
class UserData;

/// Сервис управления пользователями
class UsersService : public QObject {
	Q_OBJECT
public:
	explicit UsersService(
		IUsersDataProvider* usersDataProvider,
		ImagesService* imagesService,
		QObject* parent = nullptr);

	~UsersService() override;

	/// Войти пользователя по логину и паролю
	/// Возвращает ID пользователя или nullopt при неудаче
	std::optional<QString> login(const QString& login, const QString& password);

	/// Выйти текущего пользователя
	void logout();

	/// Проверить, авторизован ли пользователь
	bool isAuthenticated() const;

	/// Получить текущего пользователя
	std::shared_ptr<UserData> currentUser() const;

	/// Получить текущего пользователя (ID)
	QString currentUserId() const;

	/// Зарегистрировать нового пользователя
	/// Возвращает ID созданного пользователя или nullopt при ошибке
	std::optional<QString> registerUser(const QString& login, const QString& password, const QString& displayName);

	/// Создать хэш пароля (для тестирования и отладки)
	static QString hashPassword(const QString& password);

signals:
	/// Сигнал об успешном входе
	void loginSuccess(const UserData& user);

	/// Сигнал о выходе
	void loggedOut();

private:
	class Private;
	std::unique_ptr<Private> d;
};
