#include "Game/widgets/textures/texture_editor_widget.h"
#include "Game/widgets/textures/zoomable_image_view.h"
#include "Game/widgets/textures/tile_set_params_panel.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "DataLayer/textures/i_textures_data_provider.h"
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>

// === TextureEditorWidget::Private ===

class TextureEditorWidget::Private {
public:
	Private(TextureEditorWidget* parent) : q(parent) {}
	TextureEditorWidget* q;

	TexturesService* texturesService = nullptr;

	// Компоненты UI
	QSplitter* splitter = nullptr;
	ZoomableImageView* previewLabel = nullptr;
	QComboBox* textureTypeCombo = nullptr;
	QPushButton* tileSetSettingsButton = nullptr;
	QListWidget* textureList = nullptr;

	// Диалог настроек тайлов
	TileSetParamsPanel* tileSetDialog = nullptr;

	// Настройки сетки
	int tileGridSizeX = 16;
	int tileGridSizeY = 16;
	int selectedTileId = -1;
	bool showTileGrid = true;

	// Пагинация
	static constexpr int kPageSize = 50;
	int currentPage = 0;
	QStringList allTextures;
	TextureType currentType = TextureType::Icon;
};

TextureEditorWidget::TextureEditorWidget(TexturesService* texturesService, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->texturesService = texturesService;

	setObjectName("TextureEditorWidget");
	setupLayout();

	// Загружаем первый тип текстур
	d->currentType = TextureType::Icon;
	const int iconIndex = d->textureTypeCombo->findData(static_cast<int>(TextureType::Icon));
	if (iconIndex >= 0) {
		d->textureTypeCombo->setCurrentIndex(iconIndex);
	}
	updateTextureList();
	updateTileSetButton();
}

TextureEditorWidget::~TextureEditorWidget() = default;

void TextureEditorWidget::setupLayout() {
	auto* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(8);

	// Создаём сплиттер
	d->splitter = new QSplitter(Qt::Horizontal, this);

	// Левая часть - превью
	auto* previewWidget = new QWidget(d->splitter);
	auto* previewLayout = new QVBoxLayout(previewWidget);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	d->previewLabel = new ZoomableImageView(previewWidget);
	d->previewLabel->setObjectName("TexturePreviewLabel");
	d->previewLabel->setStyleSheet(R"(
		#TexturePreviewLabel {
			background-color: #1a202c;
			border: 1px solid #4a5568;
			border-radius: 8px;
		}
	)");
	previewLayout->addWidget(d->previewLabel);

	// Правая часть — вертикальный контейнер: список текстур + панель параметров
	auto* rightWidget = new QWidget(d->splitter);
	auto* rightLayout = new QVBoxLayout(rightWidget);
	rightLayout->setContentsMargins(0, 0, 0, 0);
	rightLayout->setSpacing(4);

	// Выбор типа текстур
	auto* typeLabel = new QLabel("Тип текстур:", rightWidget);
	typeLabel->setStyleSheet("color: #a0aec0; font-size: 11px; padding: 2px 0;");
	rightLayout->addWidget(typeLabel);

	d->textureTypeCombo = new QComboBox(rightWidget);
	d->textureTypeCombo->setObjectName("TextureTypeCombo");
	d->textureTypeCombo->addItem("Иконки", static_cast<int>(TextureType::Icon));
	d->textureTypeCombo->addItem("Сущности", static_cast<int>(TextureType::Entity));
	d->textureTypeCombo->addItem("Предметы", static_cast<int>(TextureType::Item));
	d->textureTypeCombo->addItem("Персонажи", static_cast<int>(TextureType::Character));
	d->textureTypeCombo->addItem("Экипировка", static_cast<int>(TextureType::Equipment));
	d->textureTypeCombo->addItem("Пользователи", static_cast<int>(TextureType::Users));
	d->textureTypeCombo->addItem("Тайлы", static_cast<int>(TextureType::TileSets));

	d->textureTypeCombo->setStyleSheet(R"(
		#TextureTypeCombo {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 4px;
		}
		#TextureTypeCombo::drop-down {
			border: none;
		}
		#TextureTypeCombo QAbstractItemView {
			background-color: #2d3748;
			color: #e2e8f0;
			selection-background-color: #4a5568;
		}
	)");

	// Кнопка настроек тайлового набора (скрыта по умолчанию)
	d->tileSetSettingsButton = new QPushButton("⚙", rightWidget);
	d->tileSetSettingsButton->setFixedSize(28, 28);
	d->tileSetSettingsButton->setToolTip("Параметры тайлового набора");
	d->tileSetSettingsButton->setVisible(false);
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

	auto* typeRowLayout = new QHBoxLayout();
	typeRowLayout->addWidget(d->textureTypeCombo, 1);
	typeRowLayout->addWidget(d->tileSetSettingsButton);
	rightLayout->addLayout(typeRowLayout);

	// Список текстур с ленивой загрузкой
	d->textureList = new QListWidget(rightWidget);
	d->textureList->setObjectName("TextureList");
	d->textureList->setStyleSheet(R"(
		#TextureList {
			background-color: #1a202c;
			border: 1px solid #4a5568;
			border-radius: 4px;
		}
		#TextureList::item {
			padding: 6px;
			border-bottom: 1px solid #2d3748;
		}
		#TextureList::item:selected {
			background-color: #4a5568;
		}
		#TextureList::item:hover {
			background-color: #2d3748;
		}
		#TextureList QScrollBar:vertical {
			background-color: #2d3748;
			width: 8px;
			border-radius: 4px;
		}
		#TextureList QScrollBar::handle:vertical {
			background-color: #4a5568;
			border-radius: 4px;
			min-height: 20px;
		}
		#TextureList QScrollBar::handle:vertical:hover {
			background-color: #718096;
		}
		#TextureList QScrollBar::add-line:vertical, #TextureList QScrollBar::sub-line:vertical {
			height: 0px;
		}
	)");
	rightLayout->addWidget(d->textureList, 1);

	// Подключаем сплиттер
	mainLayout->addWidget(d->splitter);

	// Устанавливаем пропорции сплиттера (70% превью, 30% список)
	d->splitter->setStretchFactor(0, 3);
	d->splitter->setStretchFactor(1, 1);

	// Сигналы
	connect(d->textureTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, &TextureEditorWidget::onTextureTypeChanged);
	connect(d->textureList, &QListWidget::itemClicked,
	        this, [this](QListWidgetItem* item) {
		onTextureItemSelected(item);
	});
	connect(d->textureList->verticalScrollBar(), &QScrollBar::valueChanged,
	        this, [this](int value) {
		// Ленивая загрузка при прокрутке
		if (value >= d->textureList->verticalScrollBar()->maximum() - 5) {
			loadTexturesPage();
		}
	});
	connect(d->tileSetSettingsButton, &QPushButton::clicked,
	        this, &TextureEditorWidget::openTileSetSettings);
	connect(d->previewLabel, &ZoomableImageView::tileClicked,
	        this, &TextureEditorWidget::onTileClicked);
}
void TextureEditorWidget::onTextureTypeChanged(int index) {
	d->currentType = static_cast<TextureType>(d->textureTypeCombo->itemData(index).toInt());
	d->currentPage = 0;
	d->allTextures.clear();
	d->textureList->clear();
	updateTextureList();
	updateTileSetButton();
}

void TextureEditorWidget::onTextureItemSelected(QListWidgetItem* current) {
	if (current) {
		const QString fileName = current->text();
		updatePreview(fileName);
		emit textureSelected(d->currentType, fileName);
	}
}

void TextureEditorWidget::updateTextureList() {
	d->allTextures = d->texturesService->listTextures(d->currentType);
	d->currentPage = 0;
	d->textureList->clear();
	loadTexturesPage();
}

void TextureEditorWidget::loadTexturesPage() {
	const int startIdx = d->currentPage * Private::kPageSize;
	if (startIdx >= d->allTextures.size()) {
		return; // Все данные уже загружены
	}

	const int endIdx = qMin(startIdx + Private::kPageSize, d->allTextures.size());

	// Блокируем сигналы, чтобы не вызывать onTextureItemSelected
	d->textureList->blockSignals(true);
	for (int i = startIdx; i < endIdx; ++i) {
		auto* item = new QListWidgetItem(d->allTextures[i]);
		d->textureList->addItem(item);
	}
	d->textureList->blockSignals(false);
	d->currentPage++;
}

void TextureEditorWidget::updatePreview(const QString& fileName) {
	const auto pixmap = d->texturesService->getTexture(fileName, d->currentType, "texture-editor");
	if (!pixmap.isNull()) {
		d->previewLabel->setPixmap(pixmap);
	} else {
		d->previewLabel->setPixmap(QPixmap());
	}
}

void TextureEditorWidget::updateTileSetButton() {
	const bool show = (d->currentType == TextureType::TileSets);
	d->tileSetSettingsButton->setVisible(show);

	// Применяем настройки сетки к превью
	if (show) {
		d->previewLabel->setGridEnabled(d->showTileGrid);
		d->previewLabel->setGridSize(d->tileGridSizeX, d->tileGridSizeY);
		d->previewLabel->setSelectedTileId(d->selectedTileId);
	} else {
		d->previewLabel->setGridEnabled(false);
		d->selectedTileId = -1;
	}
}

void TextureEditorWidget::openTileSetSettings() {
	if (d->currentType != TextureType::TileSets) {
		return;
	}

	// Удаляем старый диалог если есть
	if (d->tileSetDialog) {
		d->tileSetDialog->deleteLater();
		d->tileSetDialog = nullptr;
	}

	d->tileSetDialog = new TileSetParamsPanel(this);
	d->tileSetDialog->setResult(QDialog::Rejected);

	// Подключаем сигнал
	connect(d->tileSetDialog, &TileSetParamsPanel::settingsApplied,
	        this, &TextureEditorWidget::onTileSetSettingsApplied);

	d->tileSetDialog->exec();
}

void TextureEditorWidget::onTileSetSettingsApplied(int gridSizeX, int gridSizeY, bool showGrid) {
	d->tileGridSizeX = gridSizeX;
	d->tileGridSizeY = gridSizeY;
	d->showTileGrid = showGrid;
	d->selectedTileId = -1;

	// Обновляем превью
	d->previewLabel->setGridEnabled(showGrid);
	d->previewLabel->setGridSize(gridSizeX, gridSizeY);
	d->previewLabel->setSelectedTileId(-1);
}

void TextureEditorWidget::onTileClicked(int tileId) {
	if (d->currentType == TextureType::TileSets) {
		d->selectedTileId = tileId;
		d->previewLabel->setSelectedTileId(tileId);
		emit tileSelected(tileId);
	}
}
