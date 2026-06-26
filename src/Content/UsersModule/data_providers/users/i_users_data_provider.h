#pragma once
#include <QString>
#include <set>
#include <memory>

class UserData;

/// Интерфейс провайдера данных пользователей
class IUsersDataProvider {
public:
	virtual ~IUsersDataProvider() = default;

	/// Загрузить пользователя по ID
	virtual std::shared_ptr<UserData> loadUser(const QString& loginHash) const = 0;

	/// Сохранить пользователя
	virtual bool saveUser(const std::shared_ptr<UserData> user) = 0;

	/// Удалить пользователя
	virtual bool deleteUser(const QString& loginHash) = 0;

	// Проверить наличие пользователя
	virtual bool containsUser(const QString& loginHash) = 0;

	virtual std::set<QString> permissions(const QString& id) const = 0;
};
