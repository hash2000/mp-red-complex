#pragma once
#include <QWidget>
#include <memory>

class UsersRegistryService;
class QLineEdit;
class QPushButton;
class QLabel;

/// Виджет регистрации нового пользователя
class RegisterWidget : public QWidget {
	Q_OBJECT
public:
	explicit RegisterWidget(UsersRegistryService* usersRegistryService, QWidget* parent = nullptr);
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
