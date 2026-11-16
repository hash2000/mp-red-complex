#include "Content/UsersModule/widgets/register_widget.h"
#include "Content/UsersModule/services/users_registry_service.h"
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
	UsersRegistryService* usersRegistryService;

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

RegisterWidget::RegisterWidget(UsersRegistryService* usersRegistryService, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QDialog(parent) {
	d->usersRegistryService = usersRegistryService;

	setWindowTitle("Регистрация");
	setModal(true);

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
	d->titleLabel->setObjectName(QStringLiteral("AuthTitle"));
	d->titleLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(d->titleLabel);

	// Разделитель
	auto line = new QFrame();
	line->setObjectName(QStringLiteral("AuthSeparator"));
	line->setFrameShape(QFrame::HLine);
	line->setFixedHeight(2);
	mainLayout->addWidget(line);

	// Форма регистрации
	auto formLayout = new QFormLayout();
	formLayout->setSpacing(12);
	formLayout->setLabelAlignment(Qt::AlignRight);

	// Логин
	d->loginEdit = new QLineEdit();
	d->loginEdit->setObjectName(QStringLiteral("AuthInput"));
	d->loginEdit->setPlaceholderText("Придумайте логин");
	d->loginEdit->setFixedHeight(40);
	formLayout->addRow("Логин:", d->loginEdit);

	// Пароль
	d->passwordEdit = new QLineEdit();
	d->passwordEdit->setObjectName(QStringLiteral("AuthInput"));
	d->passwordEdit->setPlaceholderText("Придумайте пароль");
	d->passwordEdit->setEchoMode(QLineEdit::Password);
	d->passwordEdit->setFixedHeight(40);
	formLayout->addRow("Пароль:", d->passwordEdit);

	// Никнейм
	d->nicknameEdit = new QLineEdit();
	d->nicknameEdit->setObjectName(QStringLiteral("AuthInput"));
	d->nicknameEdit->setPlaceholderText("Ваше отображаемое имя");
	d->nicknameEdit->setFixedHeight(40);
	formLayout->addRow("Никнейм:", d->nicknameEdit);

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

	// Кнопка регистрации
	d->registerButton = new QPushButton("Зарегистрироваться");
	d->registerButton->setObjectName(QStringLiteral("AuthPrimaryButton"));
	d->registerButton->setFixedHeight(44);
	buttonLayout->addWidget(d->registerButton, 1);

	// Кнопка отмены
	d->cancelButton = new QPushButton("Отмена");
	d->cancelButton->setObjectName(QStringLiteral("AuthNeutralButton"));
	d->cancelButton->setFixedHeight(44);
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

	QStringList seedPhrase;
	auto registerResult = d->usersRegistryService->registerUser(login, password, seedPhrase);
	if (registerResult) {
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
