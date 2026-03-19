#pragma once
#include <QString>
#include <QHash>
#include <optional>

/// Данные пользователя
struct UserData {
	QString id;             // Уникальный идентификатор
	QString login;          // Логин
	QString passwordHash;   // Хэш пароля
	QString displayName;    // Отображаемое имя
	QHash<QString, QString> metadata; // Дополнительные данные
};

/// Интерфейс провайдера данных пользователей
class IUsersDataProvider {
public:
	virtual ~IUsersDataProvider() = default;

	/// Загрузить всех пользователей
	virtual bool loadAllUsers(QHash<QString, UserData>& users) const = 0;

	/// Загрузить пользователя по ID
	virtual std::optional<UserData> loadUser(const QString& userId) const = 0;

	/// Сохранить пользователя
	virtual bool saveUser(const UserData& user) = 0;

	/// Удалить пользователя
	virtual bool deleteUser(const QString& userId) = 0;

	/// Найти пользователя по логину
	virtual std::optional<UserData> findUserByLogin(const QString& login) const = 0;
};
