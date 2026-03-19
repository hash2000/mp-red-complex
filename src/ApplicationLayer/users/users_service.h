#pragma once
#include "DataLayer/users/i_users_data_provider.h"
#include <QObject>
#include <memory>
#include <optional>

/// Сервис управления пользователями
class UsersService : public QObject {
	Q_OBJECT
public:
	explicit UsersService(std::unique_ptr<IUsersDataProvider> dataProvider, QObject* parent = nullptr);
	~UsersService() override;

	/// Войти пользователя по логину и паролю
	/// Возвращает ID пользователя или nullopt при неудаче
	std::optional<QString> login(const QString& login, const QString& password);

	/// Выйти текущего пользователя
	void logout();

	/// Проверить, авторизован ли пользователь
	bool isAuthenticated() const;

	/// Получить текущего пользователя
	std::optional<UserData> currentUser() const;

	/// Получить текущего пользователя (ID)
	QString currentUserId() const;

	/// Зарегистрировать нового пользователя
	/// Возвращает ID созданного пользователя или nullopt при ошибке
	std::optional<QString> registerUser(const QString& login, const QString& password, const QString& displayName = QString());

	/// Создать хэш пароля (для тестирования и отладки)
	static QString hashPassword(const QString& password);

signals:
	/// Сигнал об успешном входе
	void loginSuccess(const QString& userId);

	/// Сигнал о выходе
	void loggedOut();

private:
	class Private;
	std::unique_ptr<Private> d;
};
