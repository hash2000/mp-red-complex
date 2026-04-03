#pragma once
#include "DataLayer/users/i_users_data_provider.h"
#include "DataLayer/characters/i_character_data_provider.h"
#include <QObject>
#include <memory>
#include <optional>
#include <map>
#include <list>

class CharacterItemHandler;
class TexturesService;

/// Сервис управления пользователями
class UsersService : public QObject {
	Q_OBJECT
public:
	explicit UsersService(
		IUsersDataProvider* usersDataProvider,
		ICharacterDataProvider* characterDataProvider,
		TexturesService* texturesService,
		QObject* parent = nullptr);
	~UsersService() override;

	/// Получить сервис текстур
	TexturesService* texturesService() const;

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
	std::optional<QString> registerUser(const QString& login, const QString& password, const QString& displayName);

	/// Получить персонажа текущего пользователя по ID
	/// Возвращает указатель на CharacterItemHandler или nullptr если персонаж не найден
	CharacterItemHandler* getCharacter(const QUuid& characterId) const;

	/// Получить список всех идентификаторов персонажей текущего пользователя
	std::list<QUuid> getAllCharacterIds() const;

	/// Получить идентификатор сундука текущего аккаунта
	/// Возвращает QUuid или пустой QUuid если пользователь не авторизован
	QUuid getChestId() const;

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
