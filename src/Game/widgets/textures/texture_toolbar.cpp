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

	// Текущий тип текстур
	TextureType currentType = TextureType::Icon;
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

	// Добавляем кнопку в layout
	d->mainLayout->addWidget(d->tileSetSettingsButton);
	d->mainLayout->addStretch();  // Растяжка, чтобы кнопки были прижаты к верху

	// Подключаем сигнал
	connect(d->tileSetSettingsButton, &QPushButton::clicked,
	        this, &TextureToolbar::tileSetSettingsRequested);
}

void TextureToolbar::setTextureType(TextureType type) {
	d->currentType = type;
	updateButtonsVisibility();
}

void TextureToolbar::updateButtonsVisibility() {
	// Показываем кнопку настроек только для TileSets
	const bool show = (d->currentType == TextureType::TileSets);
	d->tileSetSettingsButton->setVisible(show);
}
