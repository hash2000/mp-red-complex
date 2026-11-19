#pragma once
#include <QFrame>
#include <memory>

class UsersService;
class QLabel;
class QVBoxLayout;
class QScrollArea;

/// Основной виджет профиля пользователя с заголовком и списком персонажей
class UserWidget : public QFrame {
	Q_OBJECT
public:
	explicit UserWidget(UsersService* usersService, QWidget* parent = nullptr);
	~UserWidget() override;

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	void setupLayout();
	void loadUserData();

private:
	class Private;
	std::unique_ptr<Private> d;
};
