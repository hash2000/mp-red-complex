#include "Game/widgets/textures/texture_toolbar.h"
#include "DataLayer/textures/i_textures_data_provider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

class TextureToolbar::Private {
public:
	Private(TextureToolbar* parent) : q(parent) {}
	TextureToolbar* q;

	// Контейнер для кнопок
	QVBoxLayout* mainLayout = nullptr;

	// Кнопки действий (для разных TextureType)
	QPushButton* tileSetSettingsButton = nullptr;
	QPushButton* groupsListButton = nullptr;
	QPushButton* groupTilesButton = nullptr;
	QPushButton* ungroupTilesButton = nullptr;

	// Текущий тип текстур
	TextureType currentType = TextureType::Icon;

	// Выбранные тайлы (для управления доступностью кнопок)
	QList<int> selectedTileIds;
};

TextureToolbar::TextureToolbar(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	setObjectName("TextureToolbar");
	setFixedWidth(36);  // Фиксированная ширина, не растягивается
	setStyleSheet(R"(
		#TextureToolbar {
			background-color: #2d3748;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 4px;
		}
	)");

	setupLayout();
}

TextureToolbar::~TextureToolbar() = default;

void TextureToolbar::setupLayout() {
	d->mainLayout = new QVBoxLayout(this);
	d->mainLayout->setContentsMargins(4, 8, 4, 8);
	d->mainLayout->setSpacing(6);

	// === Кнопка "Параметры тайлового набора" ===
	d->tileSetSettingsButton = new QPushButton("⚙", this);
	d->tileSetSettingsButton->setFixedSize(28, 28);
	d->tileSetSettingsButton->setToolTip("Параметры тайлового набора");
	d->tileSetSettingsButton->setVisible(false);  // Скрыта по умолчанию
	d->tileSetSettingsButton->setStyleSheet(R"(
		QPushButton {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			font-size: 14px;
		}
		QPushButton:hover {
			background-color: #4a5568;
			border: 1px solid #718096;
		}
		QPushButton:pressed {
			background-color: #1a202c;
		}
	)");

	// === Кнопка "Группы тайлов" ===
	d->groupsListButton = new QPushButton("📋", this);
	d->groupsListButton->setFixedSize(28, 28);
	d->groupsListButton->setToolTip("Группы тайлов");
	d->groupsListButton->setVisible(false);  // Скрыта по умолчанию
	d->groupsListButton->setStyleSheet(R"(
		QPushButton {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			font-size: 14px;
		}
		QPushButton:hover {
			background-color: #4a5568;
			border: 1px solid #718096;
		}
		QPushButton:pressed {
			background-color: #1a202c;
		}
	)");

	// === Кнопка "Сгруппировать тайлы" ===
	d->groupTilesButton = new QPushButton("📦", this);
	d->groupTilesButton->setFixedSize(28, 28);
	d->groupTilesButton->setToolTip("Сгруппировать выбранные тайлы");
	d->groupTilesButton->setVisible(false);  // Скрыта по умолчанию
	d->groupTilesButton->setEnabled(false);  // Неактивна без выбранных тайлов
	d->groupTilesButton->setStyleSheet(R"(
		QPushButton {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			font-size: 14px;
		}
		QPushButton:hover:enabled {
			background-color: #4a5568;
			border: 1px solid #718096;
		}
		QPushButton:pressed:enabled {
			background-color: #1a202c;
		}
		QPushButton:disabled {
			color: #718096;
		}
	)");

	// === Кнопка "Разгруппировать тайлы" ===
	d->ungroupTilesButton = new QPushButton("📂", this);
	d->ungroupTilesButton->setFixedSize(28, 28);
	d->ungroupTilesButton->setToolTip("Разгруппировать выбранные тайлы");
	d->ungroupTilesButton->setVisible(false);  // Скрыта по умолчанию
	d->ungroupTilesButton->setEnabled(false);  // Неактивна без выбранных тайлов
	d->ungroupTilesButton->setStyleSheet(R"(
		QPushButton {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			font-size: 14px;
		}
		QPushButton:hover:enabled {
			background-color: #4a5568;
			border: 1px solid #718096;
		}
		QPushButton:pressed:enabled {
			background-color: #1a202c;
		}
		QPushButton:disabled {
			color: #718096;
		}
	)");

	// Добавляем кнопки в layout
	d->mainLayout->addWidget(d->tileSetSettingsButton);
	d->mainLayout->addWidget(d->groupsListButton);
	d->mainLayout->addWidget(d->groupTilesButton);
	d->mainLayout->addWidget(d->ungroupTilesButton);
	d->mainLayout->addStretch();  // Растяжка, чтобы кнопки были прижаты к верху

	// Подключаем сигналы
	connect(d->tileSetSettingsButton, &QPushButton::clicked,
	        this, &TextureToolbar::tileSetSettingsRequested);
	connect(d->groupsListButton, &QPushButton::clicked,
	        this, &TextureToolbar::groupsListRequested);
	connect(d->groupTilesButton, &QPushButton::clicked,
	        this, &TextureToolbar::groupTilesRequested);
	connect(d->ungroupTilesButton, &QPushButton::clicked,
	        this, &TextureToolbar::ungroupTilesRequested);
}

void TextureToolbar::setTextureType(TextureType type) {
	d->currentType = type;
	updateButtonsVisibility();
}

void TextureToolbar::setSelectedTiles(const QList<int>& tileIds) {
	d->selectedTileIds = tileIds;
	updateButtonsVisibility();
}

void TextureToolbar::updateButtonsVisibility() {
	// Показываем кнопки только для TileSets
	const bool show = (d->currentType == TextureType::TileSets);
	d->tileSetSettingsButton->setVisible(show);
	d->groupsListButton->setVisible(show);
	d->groupTilesButton->setVisible(show);
	d->ungroupTilesButton->setVisible(show);

	// Активируем кнопки группировки только если есть выбранные тайлы
	const bool hasSelection = !d->selectedTileIds.isEmpty();
	d->groupTilesButton->setEnabled(hasSelection);
	d->ungroupTilesButton->setEnabled(hasSelection);
}
