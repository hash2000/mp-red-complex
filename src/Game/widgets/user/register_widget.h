#pragma once
#include <QDialog>
#include <memory>

class UsersService;
class QLineEdit;
class QPushButton;
class QLabel;

/// Виджет регистрации нового пользователя
class RegisterWidget : public QDialog {
	Q_OBJECT
public:
	explicit RegisterWidget(UsersService* usersService, QWidget* parent = nullptr);
	~RegisterWidget() override;

	/// Получить введённый логин
	QString login() const;

	/// Получить введённый пароль
	QString password() const;

signals:
	/// Сигнал об успешной регистрации
	void registerSuccess();

private slots:
	void onRegisterClicked();
	void onCancelClicked();

private:
	void setupLayout();

	class Private;
	std::unique_ptr<Private> d;
};
