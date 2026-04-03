#pragma once
#include <QWidget>
#include <memory>

class UsersService;
class CharacterItemHandler;
class QLabel;
class QPushButton;
class TexturesService;

/// Виджет одной записи персонажа в списке
class CharacterEntryWidget : public QWidget {
	Q_OBJECT
public:
	explicit CharacterEntryWidget(
		UsersService* usersService,
		TexturesService* texturesService, 
		const QUuid& characterId,
		QWidget* parent = nullptr);
	~CharacterEntryWidget() override;

signals:
	void equipmentClicked(const QUuid& characterId);
	void inventoryClicked(const QUuid& characterId);

private:
	void setupLayout();
	void loadCharacterData();

	class Private;
	std::unique_ptr<Private> d;
};
