#pragma once
#include <QObject>
#include <list>
#include <memory>

class Character;
class UsersService;
class ICharacterDataProvider;
class ImagesService;

class CharactersService : public QObject {
	Q_OBJECT
public:

	explicit CharactersService(
		ICharacterDataProvider* characterDataProvider,
		UsersService* usersService,
		ImagesService* imagesService,
		QObject* parent = nullptr);

	~CharactersService() override;

	std::shared_ptr<Character> character(const QUuid& characterId) const;
	std::list<std::shared_ptr<Character>> charactersByUser(const QString& userId) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
