#include "Game/widgets/user/user_widget.h"
#include "Game/widgets/user/character_entry_widget.h"
#include "ApplicationLayer/users/users_service.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "DataLayer/users/user.h"
#include "DataLayer/textures/i_textures_data_provider.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>

class UserWidget::Private {
public:
	Private(UserWidget* parent)
		: q(parent) {
	}

	UserWidget* q;
	UsersService* usersService = nullptr;
	TexturesService* texturesService = nullptr;
	std::optional<UserData> currentUser;

	// Элементы заголовка
	QLabel* userIconLabel = nullptr;
	QLabel* userIdLabel = nullptr;
	QLabel* displayNameLabel = nullptr;

	// Контейнер для персонажей
	QVBoxLayout* charactersLayout = nullptr;
	QWidget* charactersContainer = nullptr;
	QScrollArea* scrollArea = nullptr;

	// Список виджетов персонажей
	std::list<CharacterEntryWidget*> characterWidgets;
};

UserWidget::UserWidget(UsersService* usersService, TexturesService* texturesService, QWidget* parent)
	: QFrame(parent)
	, d(std::make_unique<Private>(this)) {
	d->usersService = usersService;
	d->texturesService = texturesService;

	setObjectName("UserWidget");
	setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	setLineWidth(1);

	setStyleSheet(R"(
        #UserWidget {
            background-color: #1a202c;
            border: 1px solid #4a5568;
            border-radius: 8px;
            padding: 12px;
        }
    )");

	setupLayout();
	loadUserData();

	// Подключаемся к сигналу выхода
	connect(d->usersService, &UsersService::loggedOut, this, [this]() {
		clearCharacters();
	});
}

UserWidget::~UserWidget() = default;

void UserWidget::setupLayout() {
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(12);

	// === Заголовок пользователя ===
	auto* headerFrame = new QFrame(this);
	headerFrame->setFrameStyle(QFrame::StyledPanel);
	headerFrame->setStyleSheet(
		"QFrame { "
		"  background-color: #2d3748; "
		"  border: 1px solid #4a5568; "
		"  border-radius: 6px; "
		"  padding: 8px; "
		"}"
	);

	auto* headerLayout = new QHBoxLayout(headerFrame);
	headerLayout->setSpacing(12);
	headerLayout->setContentsMargins(10, 6, 10, 6);

	// Иконка пользователя (слева)
	d->userIconLabel = new QLabel(headerFrame);
	d->userIconLabel->setFixedSize(48, 48);
	d->userIconLabel->setScaledContents(false);
	d->userIconLabel->setStyleSheet(
		"background-color: #1a202c; "
		"border: 1px solid #4a5568; "
		"border-radius: 6px;"
	);
	headerLayout->addWidget(d->userIconLabel, 0, Qt::AlignVCenter);

	// Правая часть заголовка
	auto* infoLayout = new QVBoxLayout();
	infoLayout->setSpacing(4);

	// ID пользователя (серое поле)
	d->userIdLabel = new QLabel(headerFrame);
	d->userIdLabel->setStyleSheet(
		"background-color: #1a202c; "
		"color: #a0aec0; "
		"padding: 2px 8px; "
		"border: 1px solid #4a5568; "
		"border-radius: 4px; "
		"font-size: 10px; "
		"font-family: monospace;"
	);
	d->userIdLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	infoLayout->addWidget(d->userIdLabel);

	// DisplayName
	d->displayNameLabel = new QLabel(headerFrame);
	d->displayNameLabel->setStyleSheet(
		"color: #e2e8f0; "
		"font-size: 16px; "
		"font-weight: bold;"
	);
	infoLayout->addWidget(d->displayNameLabel);

	infoLayout->addStretch();

	headerLayout->addLayout(infoLayout, 1);

	mainLayout->addWidget(headerFrame, 0);

	// === Область прокрутки для персонажей ===
	d->scrollArea = new QScrollArea(this);
	d->scrollArea->setWidgetResizable(true);
	d->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	d->scrollArea->setStyleSheet(
		"QScrollArea { "
		"  background-color: #1a202c; "
		"  border: 1px solid #4a5568; "
		"  border-radius: 6px; "
		"}"
		"QScrollBar:vertical { "
		"  background-color: #2d3748; "
		"  width: 10px; "
		"  border-radius: 5px; "
		"}"
		"QScrollBar::handle:vertical { "
		"  background-color: #4a5568; "
		"  border-radius: 5px; "
		"  min-height: 20px; "
		"}"
		"QScrollBar::handle:vertical:hover { "
		"  background-color: #718096; "
		"}"
		"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
		"  height: 0px; "
		"}"
	);

	// Контейнер для персонажей внутри прокрутки
	d->charactersContainer = new QWidget();
	d->charactersContainer->setStyleSheet("background-color: #1a202c;");
	d->charactersLayout = new QVBoxLayout(d->charactersContainer);
	d->charactersLayout->setContentsMargins(8, 8, 8, 8);
	d->charactersLayout->setSpacing(6);
	d->charactersLayout->addStretch(); // Чтобы элементы прижимались к верху

	d->scrollArea->setWidget(d->charactersContainer);

	mainLayout->addWidget(d->scrollArea, 1);
}

void UserWidget::loadUserData() {
	d->currentUser = d->usersService->currentUser();
	if (!d->currentUser.has_value()) {
		d->displayNameLabel->setText("Не авторизован");
		d->userIdLabel->setText("N/A");
		return;
	}

	const auto& user = d->currentUser.value();

	// DisplayName
	d->displayNameLabel->setText(user.displayName.isEmpty() ? "Пользователь" : user.displayName);

	// ID (показываем сокращённую версию)
	QString shortId = user.loginHash;
	if (shortId.length() > 12) {
		shortId = shortId.left(12) + "...";
	}
	d->userIdLabel->setText(shortId);

	// Иконка пользователя
	if (!user.iconPath.isEmpty() && d->texturesService) {
		auto pixmap = d->texturesService->getTexture(user.iconPath, TextureType::Icon);
		if (!pixmap.isNull()) {
			d->userIconLabel->setPixmap(pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
	}

	// Загружаем персонажей
	loadCharacters();
}

void UserWidget::loadCharacters() {
	clearCharacters();

	const auto characterIds = d->usersService->getAllCharacterIds();
	if (characterIds.empty()) {
		auto* noCharsLabel = new QLabel("Нет персонажей", d->charactersContainer);
		noCharsLabel->setStyleSheet("color: #718096; padding: 20px; font-size: 13px;");
		noCharsLabel->setAlignment(Qt::AlignCenter);
		d->charactersLayout->insertWidget(0, noCharsLabel);
		return;
	}

	for (const auto& charId : characterIds) {
		auto* charWidget = new CharacterEntryWidget(
			d->usersService,
			d->texturesService,
			charId,
			d->charactersContainer);

		// Подключаем сигналы кнопок
		connect(charWidget, &CharacterEntryWidget::equipmentClicked,
			this, &UserWidget::equipmentRequested);
		connect(charWidget, &CharacterEntryWidget::inventoryClicked,
			this, &UserWidget::inventoryRequested);

		d->characterWidgets.push_back(charWidget);
		d->charactersLayout->insertWidget(d->charactersLayout->count() - 1, charWidget);

		// Добавим разделитель
		if (charId != characterIds.back()) {
			auto* line = new QFrame(d->charactersContainer);
			line->setFrameShape(QFrame::HLine);
			line->setStyleSheet("background-color: #4a5568;");
			d->charactersLayout->insertWidget(d->charactersLayout->count() - 1, line);
		}
	}
}

void UserWidget::clearCharacters() {
	// Удаляем все виджеты персонажей
	for (auto* widget : d->characterWidgets) {
		widget->deleteLater();
	}
	d->characterWidgets.clear();

	// Удаляем разделители и лейблы "нет персонажей"
	QLayoutItem* child;
	while ((child = d->charactersLayout->takeAt(0)) != nullptr) {
		if (child->widget()) {
			child->widget()->deleteLater();
		}
		delete child;
	}

	// Возвращаем stretch
	d->charactersLayout->addStretch();
}
