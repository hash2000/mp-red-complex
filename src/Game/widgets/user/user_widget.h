#pragma once
#include <QFrame>
#include <memory>

class UsersService;
class TexturesService;
class QLabel;
class QVBoxLayout;
class QScrollArea;

/// Основной виджет профиля пользователя с заголовком и списком персонажей
class UserWidget : public QFrame {
	Q_OBJECT
public:
	explicit UserWidget(UsersService* usersService, TexturesService* texturesService, QWidget* parent = nullptr);
	~UserWidget() override;

signals:
	void equipmentRequested(const QUuid& characterId);
	void specificationsRequested(const QUuid& characterId);

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	void setupLayout();
	void loadUserData();
	void loadCharacters();
	void clearCharacters();
	void updateCharactersContainerWidth();

	class Private;
	std::unique_ptr<Private> d;
};
