#include "Game/widgets/user/register_widget.h"
#include "ApplicationLayer/users/users_service.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

class RegisterWidget::Private {
public:
	Private(RegisterWidget* parent)
		: q(parent) {
	}

	RegisterWidget* q;
	UsersService* usersService;

	QLineEdit* loginEdit = nullptr;
	QLineEdit* passwordEdit = nullptr;
	QLineEdit* nicknameEdit = nullptr;
	QPushButton* registerButton = nullptr;
	QPushButton* cancelButton = nullptr;
	QLabel* titleLabel = nullptr;
	QLabel* errorLabel = nullptr;

	QString resultLogin;
	QString resultPassword;
};

RegisterWidget::RegisterWidget(UsersService* usersService, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QDialog(parent) {
	d->usersService = usersService;

	setWindowTitle("Регистрация");
	setModal(true);
	setStyleSheet(R"(
		RegisterWidget {
			background-color: #0f172a;
		}
	)");

	setupLayout();
}

RegisterWidget::~RegisterWidget() = default;

QString RegisterWidget::login() const {
	return d->resultLogin;
}

QString RegisterWidget::password() const {
	return d->resultPassword;
}

void RegisterWidget::setupLayout() {
	auto mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(40, 40, 40, 40);
	mainLayout->setSpacing(20);

	// Заголовок
	d->titleLabel = new QLabel("Регистрация");
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

	// Форма регистрации
	auto formLayout = new QFormLayout();
	formLayout->setSpacing(12);
	formLayout->setLabelAlignment(Qt::AlignRight);

	// Логин
	d->loginEdit = new QLineEdit();
	d->loginEdit->setPlaceholderText("Придумайте логин");
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
	d->passwordEdit->setPlaceholderText("Придумайте пароль");
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

	// Никнейм
	d->nicknameEdit = new QLineEdit();
	d->nicknameEdit->setPlaceholderText("Ваше отображаемое имя");
	d->nicknameEdit->setFixedHeight(40);
	d->nicknameEdit->setStyleSheet(R"(
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
	formLayout->addRow("Никнейм:", d->nicknameEdit);

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

	// Кнопка регистрации
	d->registerButton = new QPushButton("Зарегистрироваться");
	d->registerButton->setFixedHeight(44);
	d->registerButton->setStyleSheet(R"(
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
	buttonLayout->addWidget(d->registerButton, 1);

	// Кнопка отмены
	d->cancelButton = new QPushButton("Отмена");
	d->cancelButton->setFixedHeight(44);
	d->cancelButton->setStyleSheet(R"(
		QPushButton {
			background-color: #4a5568;
			color: #0f172a;
			border: none;
			border-radius: 6px;
			font-weight: 600;
			font-size: 14px;
		}
		QPushButton:hover {
			background-color: #5a6678;
		}
		QPushButton:pressed {
			background-color: #6a7688;
		}
	)");
	buttonLayout->addWidget(d->cancelButton, 1);

	mainLayout->addLayout(buttonLayout);

	// Подключаем сигналы
	connect(d->registerButton, &QPushButton::clicked, this, &RegisterWidget::onRegisterClicked);
	connect(d->cancelButton, &QPushButton::clicked, this, &RegisterWidget::onCancelClicked);

	// Регистрация по Enter
	connect(d->nicknameEdit, &QLineEdit::returnPressed, this, &RegisterWidget::onRegisterClicked);

	// Устанавливаем размер
	setFixedSize(400, 380);
}

void RegisterWidget::onRegisterClicked() {
	QString login = d->loginEdit->text().trimmed();
	QString password = d->passwordEdit->text();
	QString nickname = d->nicknameEdit->text().trimmed();

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

	if (nickname.isEmpty()) {
		d->errorLabel->setText("Введите никнейм");
		d->errorLabel->show();
		return;
	}

	auto result = d->usersService->registerUser(login, password, nickname);
	if (result.has_value()) {
		d->resultLogin = login;
		d->resultPassword = password;
		accept();
		emit registerSuccess();
	}
	else {
		d->errorLabel->setText("Ошибка регистрации.");
		d->errorLabel->show();
	}
}

void RegisterWidget::onCancelClicked() {
	reject();
}
