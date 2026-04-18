#include "Game/widgets/textures/texture_editor_widget.h"
#include "Game/widgets/textures/zoomable_image_view.h"
#include "Game/widgets/textures/texture_toolbar.h"
#include "Game/widgets/textures/tile_set_params_panel.h"
#include "Game/widgets/textures/tile_groups_dialog.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "ApplicationLayer/textures/tiles_service.h"
#include "DataLayer/textures/i_textures_data_provider.h"
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QUuid>
#include <QFileInfo>

// === TextureEditorWidget::Private ===

class TextureEditorWidget::Private {
public:
	Private(TextureEditorWidget* parent) : q(parent) {}
	TextureEditorWidget* q;

	TexturesService* texturesService = nullptr;
	TilesService* tilesService = nullptr;

	// Компоненты UI
	QSplitter* splitter = nullptr;
	ZoomableImageView* previewLabel = nullptr;
	QComboBox* textureTypeCombo = nullptr;
	TextureToolbar* textureToolbar = nullptr;
	QListWidget* textureList = nullptr;

	// Диалог настроек тайлов
	TileSetParamsPanel* tileSetDialog = nullptr;

	// Диалог групп тайлов
	TileGroupsDialog* tileGroupsDialog = nullptr;

	// Настройки сетки
	int tileGridSizeX = 32;
	int tileGridSizeY = 32;
	QList<int> selectedTileIds;
	QUuid selectedGroupId;
	bool showTileGrid = true;

	// Пагинация
	static constexpr int kPageSize = 50;
	int currentPage = 0;
	QStringList allTextures;
	TextureType currentType = TextureType::TileSets;
	QString currentTexturePath;  // Текущая выбранная текстура
};

TextureEditorWidget::TextureEditorWidget(
	TexturesService* texturesService,
	TilesService* tilesService,
	QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->texturesService = texturesService;
	d->tilesService = tilesService;

	setObjectName("TextureEditorWidget");
	setupLayout();

	// Загружаем первый тип текстур
	d->currentType = TextureType::Icon;
	const int iconIndex = d->textureTypeCombo->findData(static_cast<int>(TextureType::Icon));
	if (iconIndex >= 0) {
		d->textureTypeCombo->setCurrentIndex(iconIndex);
	}
	updateTextureList();
	d->textureToolbar->setTextureType(d->currentType);
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

	// Правая часть — вертикальный контейнер: панель инструментов + список текстур + панель параметров
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

	rightLayout->addWidget(d->textureTypeCombo);

	// Горизонтальный контейнер: панель инструментов + список текстур
	auto* contentLayout = new QHBoxLayout();
	contentLayout->setSpacing(4);

	// Панель инструментов (не растягивается)
	d->textureToolbar = new TextureToolbar(rightWidget);
	contentLayout->addWidget(d->textureToolbar);

	// Список текстур с ленивой загрузкой (растягивается)
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
	contentLayout->addWidget(d->textureList, 1);

	rightLayout->addLayout(contentLayout, 1);

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
	connect(d->textureToolbar, &TextureToolbar::tileSetSettingsRequested,
	        this, &TextureEditorWidget::onTileSetSettingsRequested);
	connect(d->textureToolbar, &TextureToolbar::groupsListRequested,
	        this, &TextureEditorWidget::onTileGroupsRequested);
	connect(d->textureToolbar, &TextureToolbar::groupTilesRequested,
	        this, &TextureEditorWidget::onGroupTilesRequested);
	connect(d->textureToolbar, &TextureToolbar::ungroupTilesRequested,
	        this, &TextureEditorWidget::onUngroupTilesRequested);
	connect(d->previewLabel, &ZoomableImageView::tileClicked,
	        this, &TextureEditorWidget::onTileClicked);
	connect(d->tilesService, &TilesService::groupsChanged,
	        this, &TextureEditorWidget::onGroupsChanged);
}
void TextureEditorWidget::onTextureTypeChanged(int index) {
	d->currentType = static_cast<TextureType>(d->textureTypeCombo->itemData(index).toInt());
	d->currentPage = 0;
	d->allTextures.clear();
	d->textureList->clear();
	updateTextureList();
	d->textureToolbar->setTextureType(d->currentType);
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
	d->currentTexturePath = fileName;  // Сохраняем путь для группировки
	const auto pixmap = d->texturesService->getTexture(fileName, d->currentType);
	if (!pixmap.isNull()) {
		d->previewLabel->setPixmap(pixmap);
	} else {
		d->previewLabel->setPixmap(QPixmap());
	}

	// Для тайловых наборов загружаем метаданные сетки из файла
	if (d->currentType == TextureType::TileSets) {
		const auto metadata = d->tilesService->getTileSetMetadata(fileName);
		if (metadata.has_value()) {
			d->tileGridSizeX = metadata->gridSize.x;
			d->tileGridSizeY = metadata->gridSize.y;
		}

		// Применяем настройки сетки к превью
		d->previewLabel->setGridEnabled(d->showTileGrid);
		d->previewLabel->setGridSize(d->tileGridSizeX, d->tileGridSizeY);
		d->previewLabel->setSelectedTileIds(d->selectedTileIds);
	}
}

void TextureEditorWidget::onTileSetSettingsRequested() {
	if (d->currentType != TextureType::TileSets) {
		return;
	}

	// Удаляем старый диалог если есть
	if (d->tileSetDialog) {
		d->tileSetDialog->deleteLater();
		d->tileSetDialog = nullptr;
	}

	d->tileSetDialog = new TileSetParamsPanel(d->tileGridSizeX, d->tileGridSizeY, this);
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
	d->selectedTileIds.clear();

	// Обновляем превью
	d->previewLabel->setGridEnabled(showGrid);
	d->previewLabel->setGridSize(gridSizeX, gridSizeY);
	d->previewLabel->setSelectedTileIds({});
	d->textureToolbar->setSelectedTiles({});
}

void TextureEditorWidget::onTileGroupsRequested() {
	if (d->currentType != TextureType::TileSets || d->currentTexturePath.isEmpty()) {
		return;
	}

	// Удаляем старый диалог если есть
	if (d->tileGroupsDialog) {
		d->tileGroupsDialog->deleteLater();
		d->tileGroupsDialog = nullptr;
	}

	d->tileGroupsDialog = new TileGroupsDialog(d->tilesService, d->currentTexturePath, this);
	d->tileGroupsDialog->setResult(QDialog::Rejected);

	// Выполняем диалог
	const int result = d->tileGroupsDialog->exec();
	if (result == QDialog::Accepted) {
		const auto selectedGroup = d->tileGroupsDialog->selectedGroup();
		if (selectedGroup.has_value()) {
			// Выделяем все тайлы группы
			d->selectedTileIds = selectedGroup->tileIds;
			d->previewLabel->setSelectedTileIds(d->selectedTileIds);
			d->textureToolbar->setSelectedTiles(d->selectedTileIds);
		}
	}
}

void TextureEditorWidget::onTileClicked(int tileId, bool ctrlModifier) {
	if (d->currentType == TextureType::TileSets) {
		// Без Ctrl — проверяем, принадлежит ли тайл к группе
		const auto groupOpt = d->tilesService->getGroupContainingTile(d->currentTexturePath, tileId);
		if (groupOpt.has_value()) {
			// Тайл в группе — выделяем всю группу
			if (ctrlModifier) {
				d->selectedTileIds.append(groupOpt->tileIds);
			}
			else {
				d->selectedTileIds = groupOpt->tileIds;
			}

			d->selectedGroupId = groupOpt->id;
		} else {
			// Тайл не в группе — выделяем только его
			if (!ctrlModifier) {
				d->selectedTileIds.clear();
			}

			d->selectedTileIds.append(tileId);
			d->selectedGroupId = QUuid();
		}

		d->tilesService->setSelectionTiles(d->selectedTileIds);
		d->previewLabel->setSelectedTileIds(d->selectedTileIds);
		d->textureToolbar->setSelectedTiles(d->selectedTileIds);
	}

	// Отправляем сигнал с последним выбранным тайлом
	if (!d->selectedTileIds.isEmpty()) {
		emit tileSelected(d->selectedTileIds.last());
	}
}

void TextureEditorWidget::updateSelectedTiles(int tileId) {
	// Если тайл уже выбран - убираем его
	const int idx = d->selectedTileIds.indexOf(tileId);
	if (idx >= 0) {
		d->selectedTileIds.removeAt(idx);
	} else {
		// Добавляем к выбору
		d->selectedTileIds.append(tileId);
	}

	// Обновляем UI
	d->previewLabel->setSelectedTileIds(d->selectedTileIds);
	d->textureToolbar->setSelectedTiles(d->selectedTileIds);
}

void TextureEditorWidget::onGroupTilesRequested() {
	if (d->selectedTileIds.isEmpty() || d->currentTexturePath.isEmpty()) {
		return;
	}

	// Создаем группу с именем по умолчанию
	const QString groupName = QString("Группа %1").arg(d->selectedTileIds.size());
	const QUuid groupId = d->tilesService->createGroup(d->currentTexturePath, groupName, d->selectedTileIds);

	if (!groupId.isNull()) {
		// Очищаем выделение после успешного создания группы
		d->selectedTileIds.clear();
		d->previewLabel->setSelectedTileIds({});
		d->textureToolbar->setSelectedTiles({});
	}
}

void TextureEditorWidget::onUngroupTilesRequested() {
	if (d->selectedGroupId.isNull() || d->selectedTileIds.isEmpty()) {
		return;
	}

	d->tilesService->deleteGroup(d->selectedGroupId);
	d->selectedGroupId = QUuid();
}

void TextureEditorWidget::onGroupsChanged(const QString& texturePath) {
	if (texturePath != d->currentTexturePath) {
		return;
	}

	const auto groups = d->tilesService->getGroups(texturePath);

	d->previewLabel->setSelectedTileIds(d->selectedTileIds);
	d->textureToolbar->setSelectedTiles(d->selectedTileIds);
}
