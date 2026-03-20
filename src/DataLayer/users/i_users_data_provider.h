#pragma once
#include <QString>
#include <QHash>
#include <optional>

/// Данные пользователя
struct UserData {
	QString loginHash;
	QString passwordHash;
	QString displayName;
	QHash<QString, QString> metadata;
};

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
