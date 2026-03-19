#include "Game/widgets/user/login_widget.h"
#include "ApplicationLayer/users/users_service.h"
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
	UsersService* usersService;

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
	setStyleSheet(R"(
		LoginWidget {
			background-color: #0f172a;
		}
	)");

	setupLayout();
}

LoginWidget::~LoginWidget() = default;

void LoginWidget::setupLayout() {
	auto mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(40, 40, 40, 40);
	mainLayout->setSpacing(20);

	// Заголовок
	d->titleLabel = new QLabel("Вход в систему");
	d->titleLabel->setStyleSheet(
		"color: #e2e8f0; font-weight: 700; font-size: 24px;");
	d->titleLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(d->titleLabel);

	// Разделитель
	auto line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setStyleSheet("background-color: #4a5568;");
	line->setFixedHeight(2);
	mainLayout->addWidget(line);

	// Форма входа
	auto formLayout = new QFormLayout();
	formLayout->setSpacing(12);
	formLayout->setLabelAlignment(Qt::AlignRight);

	// Логин
	d->loginEdit = new QLineEdit();
	d->loginEdit->setPlaceholderText("Введите логин");
	d->loginEdit->setFixedHeight(40);
	d->loginEdit->setStyleSheet(R"(
		QLineEdit {
			background-color: rgba(30, 41, 59, 200);
			border: 1px solid #4a5568;
			border-radius: 6px;
			color: #e2e8f0;
			padding: 0 12px;
			font-size: 14px;
		}
		QLineEdit:focus {
			border-color: #3b82f6;
		}
	)");
	formLayout->addRow("Логин:", d->loginEdit);

	// Пароль
	d->passwordEdit = new QLineEdit();
	d->passwordEdit->setPlaceholderText("Введите пароль");
	d->passwordEdit->setEchoMode(QLineEdit::Password);
	d->passwordEdit->setFixedHeight(40);
	d->passwordEdit->setStyleSheet(R"(
		QLineEdit {
			background-color: rgba(30, 41, 59, 200);
			border: 1px solid #4a5568;
			border-radius: 6px;
			color: #e2e8f0;
			padding: 0 12px;
			font-size: 14px;
		}
		QLineEdit:focus {
			border-color: #3b82f6;
		}
	)");
	formLayout->addRow("Пароль:", d->passwordEdit);

	mainLayout->addLayout(formLayout);

	// Метка для ошибок
	d->errorLabel = new QLabel();
	d->errorLabel->setStyleSheet("color: #ef4444; font-size: 13px;");
	d->errorLabel->setWordWrap(true);
	d->errorLabel->hide();
	mainLayout->addWidget(d->errorLabel);

	// Кнопки
	auto buttonLayout = new QHBoxLayout();
	buttonLayout->setSpacing(12);

	// Кнопка входа
	d->loginButton = new QPushButton("Войти");
	d->loginButton->setFixedHeight(44);
	d->loginButton->setStyleSheet(R"(
		QPushButton {
			background-color: #10b981;
			color: #0f172a;
			border: none;
			border-radius: 6px;
			font-weight: 600;
			font-size: 14px;
		}
		QPushButton:hover {
			background-color: #0da271;
		}
		QPushButton:pressed {
			background-color: #0a8c62;
		}
	)");
	buttonLayout->addWidget(d->loginButton, 1);

	// Кнопка регистрации
	d->registerButton = new QPushButton("Регистрация");
	d->registerButton->setFixedHeight(44);
	d->registerButton->setStyleSheet(R"(
		QPushButton {
			background-color: #3b82f6;
			color: #0f172a;
			border: none;
			border-radius: 6px;
			font-weight: 600;
			font-size: 14px;
		}
		QPushButton:hover {
			background-color: #2563eb;
		}
		QPushButton:pressed {
			background-color: #1d4ed8;
		}
	)");
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

	auto result = d->usersService->login(login, password);
	if (result.has_value()) {
		d->errorLabel->hide();
		emit loginSuccess();
	}
	else {
		d->errorLabel->setText("Неверный логин или пароль");
		d->errorLabel->show();
	}
}

void LoginWidget::onRegisterClicked() {
	emit registerRequested();
}
