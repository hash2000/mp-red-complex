#include "Game/widgets/user/character_entry_widget.h"
#include "ApplicationLayer/users/users_service.h"
#include "ApplicationLayer/character/character_item_handler.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "DataLayer/textures/i_textures_data_provider.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class CharacterEntryWidget::Private {
public:
	Private(CharacterEntryWidget* parent)
		: q(parent) {
	}

	CharacterEntryWidget* q;
	UsersService* usersService = nullptr;
	TexturesService* texturesService = nullptr;
	QUuid characterId;
	CharacterItemHandler* character = nullptr;

	// UI элементы
	QLabel* iconLabel = nullptr;
	QLabel* levelLabel = nullptr;
	QLabel* nameLabel = nullptr;
	QPushButton* equipmentButton = nullptr;
	QPushButton* inventoryButton = nullptr;
};

CharacterEntryWidget::CharacterEntryWidget(
	UsersService* usersService,
	TexturesService* texturesService,
	const QUuid& characterId,
	QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->usersService = usersService;
	d->texturesService = texturesService;
	d->characterId = characterId;

	setupLayout();
	loadCharacterData();
}

CharacterEntryWidget::~CharacterEntryWidget() = default;

void CharacterEntryWidget::setupLayout() {
	auto* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(12);

	// Иконка персонажа (слева)
	d->iconLabel = new QLabel(this);
	d->iconLabel->setFixedSize(64, 64);
	d->iconLabel->setScaledContents(false);
	d->iconLabel->setStyleSheet(
		"background-color: #1a202c; "
		"border: 1px solid #4a5568; "
		"border-radius: 6px;"
	);
	mainLayout->addWidget(d->iconLabel, 0, Qt::AlignTop);

	// Правая часть (вертикальная)
	auto* rightLayout = new QVBoxLayout();
	rightLayout->setSpacing(4);

	// Верхняя строка: уровень и имя
	auto* infoLayout = new QHBoxLayout();
	infoLayout->setSpacing(8);

	// Уровень
	d->levelLabel = new QLabel(this);
	d->levelLabel->setStyleSheet(
		"background-color: #2d3748; "
		"color: #a0aec0; "
		"padding: 2px 8px; "
		"border: 1px solid #4a5568; "
		"border-radius: 4px; "
		"font-size: 11px;"
	);
	infoLayout->addWidget(d->levelLabel, 0, Qt::AlignTop);

	// Имя персонажа
	d->nameLabel = new QLabel(this);
	d->nameLabel->setStyleSheet(
		"font-size: 14px; "
		"font-weight: bold; "
		"color: #e2e8f0;"
	);
	infoLayout->addWidget(d->nameLabel, 1, Qt::AlignTop);

	rightLayout->addLayout(infoLayout);

	// Кнопки (экипировка, инвентарь)
	auto* buttonsLayout = new QHBoxLayout();
	buttonsLayout->setSpacing(6);

	d->equipmentButton = new QPushButton("Экипировка", this);
	d->equipmentButton->setFixedHeight(28);
	d->equipmentButton->setCursor(Qt::PointingHandCursor);
	d->equipmentButton->setStyleSheet(
		"QPushButton { "
		"  background-color: #2d3748; "
		"  color: #e2e8f0; "
		"  border: 1px solid #4a5568; "
		"  border-radius: 4px; "
		"  padding: 4px 12px; "
		"}"
		"QPushButton:hover { "
		"  background-color: #4a5568; "
		"  border: 1px solid #718096; "
		"}"
		"QPushButton:pressed { "
		"  background-color: #1a202c; "
		"}"
	);
	connect(d->equipmentButton, &QPushButton::clicked, this, [this]() {
		emit equipmentClicked(d->characterId);
	});
	buttonsLayout->addWidget(d->equipmentButton);

	d->inventoryButton = new QPushButton("Инвентарь", this);
	d->inventoryButton->setFixedHeight(28);
	d->inventoryButton->setCursor(Qt::PointingHandCursor);
	d->inventoryButton->setStyleSheet(
		"QPushButton { "
		"  background-color: #2d3748; "
		"  color: #e2e8f0; "
		"  border: 1px solid #4a5568; "
		"  border-radius: 4px; "
		"  padding: 4px 12px; "
		"}"
		"QPushButton:hover { "
		"  background-color: #4a5568; "
		"  border: 1px solid #718096; "
		"}"
		"QPushButton:pressed { "
		"  background-color: #1a202c; "
		"}"
	);
	connect(d->inventoryButton, &QPushButton::clicked, this, [this]() {
		emit inventoryClicked(d->characterId);
	});
	buttonsLayout->addWidget(d->inventoryButton);

	buttonsLayout->addStretch();

	rightLayout->addLayout(buttonsLayout);
	rightLayout->addStretch();

	mainLayout->addLayout(rightLayout, 1);
}

void CharacterEntryWidget::loadCharacterData() {
	d->character = d->usersService->getCharacter(d->characterId);
	if (!d->character) {
		d->nameLabel->setText("Неизвестный персонаж");
		d->levelLabel->setText("Ур. ?");
		return;
	}

	// Имя
	d->nameLabel->setText(d->character->name);

	// Уровень
	d->levelLabel->setText(QString("Ур. %1")
		.arg(d->character->level));

	// Иконка
	if (!d->character->iconPath.isEmpty() && d->texturesService) {
		auto pixmap = d->texturesService->getTexture(d->character->iconPath, TextureType::Character);
		if (!pixmap.isNull()) {
			d->iconLabel->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
	}
}
