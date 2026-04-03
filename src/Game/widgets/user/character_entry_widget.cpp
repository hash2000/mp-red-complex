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
	const QUuid& characterId,
	QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->usersService = usersService;
	d->texturesService = usersService->texturesService();
	d->characterId = characterId;

	setupLayout();
	loadCharacterData();
}

CharacterEntryWidget::~CharacterEntryWidget() = default;

void CharacterEntryWidget::setupLayout() {
	auto* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(12);

	// Иконка персонажа (слева)
	d->iconLabel = new QLabel(this);
	d->iconLabel->setFixedSize(64, 64);
	d->iconLabel->setScaledContents(false);
	d->iconLabel->setStyleSheet("background-color: #2d2d2d; border: 1px solid #555;");
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
		"background-color: #3d3d3d; "
		"color: #aaa; "
		"padding: 2px 8px; "
		"border-radius: 3px; "
		"font-size: 11px;"
	);
	infoLayout->addWidget(d->levelLabel, 0, Qt::AlignTop);

	// Имя персонажа
	d->nameLabel = new QLabel(this);
	d->nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff;");
	infoLayout->addWidget(d->nameLabel, 1, Qt::AlignTop);

	rightLayout->addLayout(infoLayout);

	// Кнопки (экипировка, инвентарь)
	auto* buttonsLayout = new QHBoxLayout();
	buttonsLayout->setSpacing(6);

	d->equipmentButton = new QPushButton("Экипировка", this);
	d->equipmentButton->setFixedHeight(28);
	connect(d->equipmentButton, &QPushButton::clicked, this, [this]() {
		emit equipmentClicked(d->characterId);
	});
	buttonsLayout->addWidget(d->equipmentButton);

	d->inventoryButton = new QPushButton("Инвентарь", this);
	d->inventoryButton->setFixedHeight(28);
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
	d->levelLabel->setText(QString("Ур. %1").arg(d->character->level));

	// Иконка
	if (!d->character->iconPath.isEmpty() && d->texturesService) {
		auto pixmap = d->texturesService->getTexture(d->character->iconPath, TextureType::Character);
		if (!pixmap.isNull()) {
			d->iconLabel->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
	}
}
