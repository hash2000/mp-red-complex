#pragma once
#include <QObject>

class Character;
class ICharacterDataProvider;
class ImagesService;

class CharactersService : public QObject {
	Q_OBJECT
public:

	explicit CharactersService(
		ICharacterDataProvider* characterDataProvider,
		ImagesService* imagesService,
		QObject* parent = nullptr);
	~CharactersService() override;

	/// Получить персонажа текущего пользователя по ID
/// Возвращает указатель на CharacterItemHandler или nullptr если персонаж не найден
	std::shared_ptr<Character> getCharacter(const QUuid& characterId) const;

	/// Получить список всех идентификаторов персонажей текущего пользователя
	std::list<QUuid> getAllCharacterIds() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
