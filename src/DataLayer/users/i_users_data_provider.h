	#pragma once
#include <QString>
#include <optional>
#include <list>

struct UserData;

/// Интерфейс провайдера данных пользователей
class IUsersDataProvider {
public:
	virtual ~IUsersDataProvider() = default;

	/// Загрузить пользователя по ID
	virtual std::optional<UserData> loadUser(const QString& loginHash) const = 0;

	/// Сохранить пользователя
	virtual bool saveUser(const UserData& loginHash) = 0;

	/// Удалить пользователя
	virtual bool deleteUser(const QString& loginHash) = 0;
};
