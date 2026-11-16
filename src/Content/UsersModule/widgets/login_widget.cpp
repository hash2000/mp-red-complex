#include "Content/UsersModule/widgets/login_widget.h"
#include "Content/UsersModule/widgets/register_widget.h"
#include "Content/UsersModule/services/users_service.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QFrame>

class LoginWidget::Private {
public:
	Private(LoginWidget* parent)
		: q(parent) {
	}

	LoginWidget* q;
	UsersService* usersService = nullptr;

	QLineEdit* loginEdit = nullptr;
	QLineEdit* passwordEdit = nullptr;
	QPushButton* loginButton = nullptr;
	QPushButton* registerButton = nullptr;
	QLabel* titleLabel = nullptr;
	QLabel* errorLabel = nullptr;
};

LoginWidget::LoginWidget(UsersService* usersService, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {
	d->usersService = usersService;

	setWindowTitle("Вход в систему");

	setupLayout();
}

LoginWidget::~LoginWidget() = default;

void LoginWidget::setupLayout() {
	auto mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(40, 40, 40, 40);
	mainLayout->setSpacing(20);

	// Заголовок
	d->titleLabel = new QLabel("Вход в систему");
	d->titleLabel->setObjectName(QStringLiteral("AuthTitle"));
	d->titleLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(d->titleLabel);

	// Разделитель
	auto line = new QFrame();
	line->setObjectName(QStringLiteral("AuthSeparator"));
	line->setFrameShape(QFrame::HLine);
	line->setFixedHeight(2);
	mainLayout->addWidget(line);

	// Форма входа
	auto formLayout = new QFormLayout();
	formLayout->setSpacing(12);
	formLayout->setLabelAlignment(Qt::AlignRight);

	// Логин
	d->loginEdit = new QLineEdit();
	d->loginEdit->setObjectName(QStringLiteral("AuthInput"));
	d->loginEdit->setPlaceholderText("Введите логин");
	d->loginEdit->setFixedHeight(40);
	formLayout->addRow("Логин:", d->loginEdit);

	// Пароль
	d->passwordEdit = new QLineEdit();
	d->passwordEdit->setObjectName(QStringLiteral("AuthInput"));
	d->passwordEdit->setPlaceholderText("Введите пароль");
	d->passwordEdit->setEchoMode(QLineEdit::Password);
	d->passwordEdit->setFixedHeight(40);
	formLayout->addRow("Пароль:", d->passwordEdit);

	mainLayout->addLayout(formLayout);

	// Метка для ошибок
	d->errorLabel = new QLabel();
	d->errorLabel->setObjectName(QStringLiteral("AuthError"));
	d->errorLabel->setWordWrap(true);
	d->errorLabel->hide();
	mainLayout->addWidget(d->errorLabel);

	// Кнопки
	auto buttonLayout = new QHBoxLayout();
	buttonLayout->setSpacing(12);

	// Кнопка входа
	d->loginButton = new QPushButton("Войти");
	d->loginButton->setObjectName(QStringLiteral("AuthPrimaryButton"));
	d->loginButton->setFixedHeight(44);
	buttonLayout->addWidget(d->loginButton, 1);

	// Кнопка регистрации
	d->registerButton = new QPushButton("Регистрация");
	d->registerButton->setObjectName(QStringLiteral("AuthSecondaryButton"));
	d->registerButton->setFixedHeight(44);
	buttonLayout->addWidget(d->registerButton, 1);

	mainLayout->addLayout(buttonLayout);
	mainLayout->addStretch();

	// Подключаем сигналы
	connect(d->loginButton, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
	connect(d->registerButton, &QPushButton::clicked, this, &LoginWidget::onRegisterClicked);

	// Вход по Enter
	connect(d->passwordEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLoginClicked);

	// Устанавливаем размер
	setFixedSize(400, 350);
}

void LoginWidget::onLoginClicked() {
	QString login = d->loginEdit->text().trimmed();
	QString password = d->passwordEdit->text();

	if (login.isEmpty()) {
		d->errorLabel->setText("Введите логин");
		d->errorLabel->show();
		return;
	}

	if (password.isEmpty()) {
		d->errorLabel->setText("Введите пароль");
		d->errorLabel->show();
		return;
	}

	//auto result = d->usersService->login(login, password);
	//if (result.has_value()) {
	//	d->errorLabel->hide();
	//	emit loginSuccess();
	//}
	//else {
	//	d->errorLabel->setText("Неверный логин или пароль");
	//	d->errorLabel->show();
	//}
}

void LoginWidget::onRegisterClicked() {
	//RegisterWidget registerDialog(d->usersService, this);
	//if (registerDialog.exec() == QDialog::Accepted) {
	//	// Заполняем поля входа новыми учётными данными
	//	d->loginEdit->setText(registerDialog.login());
	//	d->passwordEdit->setText(registerDialog.password());
	//	
	//	// Автоматически выполняем вход
	//	onLoginClicked();
	//}
}
