#include "Game/widgets/user/character_entry_widget.h"
#include "ApplicationLayer/users/users_service.h"
#include "ApplicationLayer/character/character_item_handler.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "DataLayer/images/i_images_data_provider.h"
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
	QPushButton* specificationsButton = nullptr;
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

	// Растягиваемся по горизонтали, фиксируемся по вертикали
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	setupLayout();
	loadCharacterData();
}

CharacterEntryWidget::~CharacterEntryWidget() = default;

void CharacterEntryWidget::setupLayout() {
	auto* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(6, 4, 6, 4);
	mainLayout->setSpacing(10);

	// Иконка персонажа (слева)
	d->iconLabel = new QLabel(this);
	d->iconLabel->setFixedSize(54, 54);
	d->iconLabel->setScaledContents(true);
	d->iconLabel->setStyleSheet(
		"background-color: #1a202c; "
		"border: 1px solid #4a5568; "
		"border-radius: 4px;"
	);
	mainLayout->addWidget(d->iconLabel, 0, Qt::AlignTop);

	// Центральная часть: уровень и имя
	auto* infoLayout = new QVBoxLayout();
	infoLayout->setSpacing(2);
	infoLayout->setContentsMargins(0, 0, 0, 0);

	// Уровень
	d->levelLabel = new QLabel(this);
	d->levelLabel->setStyleSheet(
		"background-color: #2d3748; "
		"color: #a0aec0; "
		"padding: 1px 6px; "
		"border: 1px solid #4a5568; "
		"border-radius: 3px; "
		"font-size: 10px;"
	);
	infoLayout->addWidget(d->levelLabel, 0, Qt::AlignLeft);

	// Имя персонажа
	d->nameLabel = new QLabel(this);
	d->nameLabel->setStyleSheet(
		"font-size: 13px; "
		"font-weight: bold; "
		"color: #e2e8f0;"
	);
	infoLayout->addWidget(d->nameLabel, 0, Qt::AlignTop);

	infoLayout->addStretch();

	mainLayout->addLayout(infoLayout, 1);

	// Кнопки справа (вертикально)
	auto* buttonsLayout = new QVBoxLayout();
	buttonsLayout->setSpacing(4);
	buttonsLayout->setContentsMargins(0, 0, 0, 0);

	// Кнопка экипировки (символ меча ⚔)
	d->equipmentButton = new QPushButton("⚔", this);
	d->equipmentButton->setFixedSize(24, 24);
	d->equipmentButton->setCursor(Qt::PointingHandCursor);
	d->equipmentButton->setToolTip("Экипировка");
	d->equipmentButton->setStyleSheet(
		"QPushButton { "
		"  background-color: #2d3748; "
		"  color: #e2e8f0; "
		"  border: 1px solid #4a5568; "
		"  border-radius: 3px; "
		"  font-size: 12px; "
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

	// Кнопка инвентаря (символ рюкзака 🎒)
	d->specificationsButton = new QPushButton("🎒", this);
	d->specificationsButton->setFixedSize(24, 24);
	d->specificationsButton->setCursor(Qt::PointingHandCursor);
	d->specificationsButton->setToolTip("Характеристики");
	d->specificationsButton->setStyleSheet(
		"QPushButton { "
		"  background-color: #2d3748; "
		"  color: #e2e8f0; "
		"  border: 1px solid #4a5568; "
		"  border-radius: 3px; "
		"  font-size: 12px; "
		"}"
		"QPushButton:hover { "
		"  background-color: #4a5568; "
		"  border: 1px solid #718096; "
		"}"
		"QPushButton:pressed { "
		"  background-color: #1a202c; "
		"}"
	);
	connect(d->specificationsButton, &QPushButton::clicked, this, [this]() {
		emit specificationsClicked(d->characterId);
	});
	buttonsLayout->addWidget(d->specificationsButton);

	buttonsLayout->addStretch();

	mainLayout->addLayout(buttonsLayout);
	mainLayout->setAlignment(Qt::AlignTop);
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
		auto pixmap = d->texturesService->getTexture(d->character->iconPath, ImageType::Character);
		if (!pixmap.isNull()) {
			d->iconLabel->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
	}
}
