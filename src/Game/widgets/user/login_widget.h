#pragma once
#include <QWidget>
#include <memory>

class UsersService;
class QLineEdit;
class QPushButton;
class QLabel;

/// Виджет формы входа
class LoginWidget : public QWidget {
	Q_OBJECT
public:
	explicit LoginWidget(UsersService* usersService, QWidget* parent = nullptr);
	~LoginWidget() override;

signals:
	/// Сигнал об успешном входе
	void loginSuccess();

	/// Сигнал о запросе регистрации
	void registerRequested();

private slots:
	void onLoginClicked();
	void onRegisterClicked();

private:
	void setupLayout();

	class Private;
	std::unique_ptr<Private> d;
};
