#include "Launcher/widgets/map_view/map_editor_widget.h"
#include "Launcher/widgets/map_view/create_map_dialog.h"
#include "Launcher/widgets/map_view/apply_map_filter_dialog.h"

#include "ApplicationLayer/maps/map_service.h"
#include "Content/TexturesModule/services/tiles_selector_service.h"
#include "Content/TexturesModule/services/textures_service.h"

#include "Libs/Graphics/textures/texture_atlas.h"
#include "Libs/Graphics/tiles/tile_renderer.h"
#include "Libs/Graphics/tiles/chunk.h"
#include "Libs/Graphics/textures/uploaded_texture.h"

#include <QToolBar>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>

class MapEditorWidget::Private {
public:
  Private(MapEditorWidget* parent) : q(parent) {}

  MapEditorWidget* q;
  MapService* mapService = nullptr;
  TilesSelectorService* tilesSelectorService = nullptr;
	TexturesService* textureService = nullptr;

  // UI компоненты
  QToolBar* toolbar = nullptr;
  QComboBox* modeComboBox = nullptr;
  QComboBox* mapComboBox = nullptr;

  // Панель свойств
  QGroupBox* propertiesGroup = nullptr;
  QLabel* tileIdLabel = nullptr;
  QLabel* positionLabel = nullptr;
  QLabel* currentMapLabel = nullptr;
  QLabel* currentAtlasLabel = nullptr;

  // Состояние
	std::optional<MapMetadata> currentMapMetadata;
	std::optional<QString> currentMapName;
	Chunk* currentChunk = nullptr;
  MapEditorMode currentMode = MapEditorMode::Draw;
  std::optional<QPoint> hoveredTile;
  bool isDrawing = false;
	bool needLoadAtlas = false;
	std::optional<UploadedTextureProperties> tilesetSettings;

	int tileRenderLayer = 0;

	static constexpr QColor selectedBorderColor = QColor(0, 255, 0, 255);
	static constexpr QColor unselectedBorderColor = QColor(255, 0, 0, 255);
};

MapEditorWidget::MapEditorWidget(
	ShadersService* shadersService,
	TexturesService* textureService,
  MapService* mapService,
  TilesSelectorService* tilesSelectorService,
  QWidget* parent)
  : MapViewBase(shadersService, parent)
  , d(std::make_unique<Private>(this)) {
	d->textureService = textureService;
  d->mapService = mapService;
  d->tilesSelectorService = tilesSelectorService;

  setupUI();

  // Подписываемся на изменения карты
  if (d->mapService) {
		connect(d->mapService, &MapService::currentMapChanged,
			this, &MapEditorWidget::onMapChanged);
  }

	connect(this, &MapViewBase::beginFrame, this, &MapEditorWidget::onBeginFrame);
	connect(this, &MapViewBase::tileClicked, this, &MapEditorWidget::onTileClicked);
	connect(this, &MapViewBase::tileHovered, this, &MapEditorWidget::onTileHovered);
}

MapEditorWidget::~MapEditorWidget() = default;

MapEditorMode MapEditorWidget::currentMode() const {
  return d->currentMode;
}

void MapEditorWidget::setMode(MapEditorMode mode) {
  if (d->currentMode != mode) {
		d->currentMode = mode;
		emit modeChanged(mode);

		// Обновляем comboBox
		if (d->modeComboBox) {
			d->modeComboBox->setCurrentIndex(static_cast<int>(mode));
		}
  }
}

void MapEditorWidget::onModeChanged(int index) {
	d->currentMode = static_cast<MapEditorMode>(d->modeComboBox->itemData(index).toInt());
  emit modeChanged(d->currentMode);
  qDebug() << "Map editor mode changed to:" << index;
}

void MapEditorWidget::onMapChanged(const QString& mapName) {
  if (d->mapComboBox && !mapName.isEmpty()) {
		d->mapComboBox->setCurrentText(mapName);
  }

  // Загружаем tilemap для карты
  if (d->mapService && !mapName.isEmpty()) {
		auto metadata = d->mapService->loadMapMetadata(mapName);
		if (metadata.has_value()) {
			// TODO: Загрузить tilemap для карты через TilesService
			qInfo() << "Loaded tilemap for map:" << mapName;
		}
  }
}

void MapEditorWidget::onTileClicked(std::optional<QPoint> point) {
  if (!point.has_value()) {
		return;
  }

  d->isDrawing = true;

  switch (d->currentMode) {
  case MapEditorMode::Draw:
		placeTile(point->x(), point->y());
		break;
  case MapEditorMode::Erase:
		eraseTile(point->x(), point->y());
		break;
  case MapEditorMode::Select:
		selectTile(point->x(), point->y());
		break;
  case MapEditorMode::Properties:
		// TODO: Редактирование свойств тайла
		break;
  }
}

void MapEditorWidget::onTileHovered(std::optional<QPoint> point) {
  d->hoveredTile = point;
  updatePropertiesPanel();
}

void MapEditorWidget::setupUI() {
  // Основной layout
  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Toolbar
  setupToolbar();
  mainLayout->addWidget(d->toolbar);

  // MapViewBase займёт основное пространство
  // (он уже является этим виджетом через наследование)

  // Панель свойств
  setupPropertiesPanel();
  mainLayout->addWidget(d->propertiesGroup);

	// Включаем отладочную отрисовку рамок чанков
	tileRenderer()->setDebugRenderPasses(TileRenderer::DebugRenderPass::ChunkBorders);
}

void MapEditorWidget::setupToolbar() {
  d->toolbar = new QToolBar("Map Editor");
  d->toolbar->setMovable(false);

  // Выбор режима
  d->toolbar->addWidget(new QLabel("Режим: "));
  d->modeComboBox = new QComboBox();
  d->modeComboBox->addItem("Рисование", static_cast<int>(MapEditorMode::Draw));
  d->modeComboBox->addItem("Стирание", static_cast<int>(MapEditorMode::Erase));
  d->modeComboBox->addItem("Выделение", static_cast<int>(MapEditorMode::Select));
  d->modeComboBox->addItem("Свойства", static_cast<int>(MapEditorMode::Properties));
  connect(d->modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &MapEditorWidget::onModeChanged);

  d->toolbar->addWidget(d->modeComboBox);

	auto* applyPixmap = new QPushButton("> атлас");
	connect(applyPixmap, &QPushButton::clicked, this, &MapEditorWidget::onApplySelectedAtlas);

	d->toolbar->addWidget(applyPixmap);

	auto* renderLayerEdit = new QLineEdit(this);
	renderLayerEdit->setValidator(new QIntValidator(1, TileRenderer::kMaxTileRenderLayer, this));
	renderLayerEdit->setFixedWidth(50);
	renderLayerEdit->setAlignment(Qt::AlignCenter);
	renderLayerEdit->setText(QString::number(tileRenderLayer()));
	connect(renderLayerEdit, &QLineEdit::textChanged, this, &MapEditorWidget::onRenderLevelEditChanged);

	d->toolbar->addWidget(renderLayerEdit);

	auto* showSelectedLayerOnly = new QCheckBox(this);
	showSelectedLayerOnly->setToolTip("Отображать только текущий слой");
	connect(showSelectedLayerOnly, &QCheckBox::toggled, this, &MapEditorWidget::onShowSelectedLayerOnly);

	d->toolbar->addWidget(showSelectedLayerOnly);

  d->toolbar->addSeparator();

  // Выбор карты
  d->toolbar->addWidget(new QLabel("Карта: "));
  d->mapComboBox = new QComboBox();

  if (d->mapService) {
		const auto maps = d->mapService->getAvailableMaps();
		d->mapComboBox->addItems(maps);
  }

	auto selectCurrentMap = [this](int index) {
		const QString mapName = d->mapComboBox->itemText(index);
		if (d->mapService) {
			d->mapService->setCurrentMap(mapName);
			d->currentMapMetadata = d->mapService->getCurrentMapMetadata();
			d->currentMapName = d->mapService->getCurrentMapName();

			if (d->currentMapMetadata.has_value()) {
				tileRenderer()->setChunkSize(d->currentMapMetadata->chunkSize);
			}
		}
	};

  connect(d->mapComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, selectCurrentMap);
  d->toolbar->addWidget(d->mapComboBox);

	const auto currentMapIndex = d->mapComboBox->currentIndex();
	selectCurrentMap(currentMapIndex);

  // Кнопка создания новой карты
  auto* newMapButton = new QPushButton("Новая карта");
  connect(newMapButton, &QPushButton::clicked, this, [this]() {
		CreateMapDialog dialog(this);
		if (dialog.exec() == QDialog::Accepted) {
			const QString mapName = dialog.getMapName();
			if (mapName.isEmpty()) {
				qWarning() << "MapEditorWidget: map name is empty";
				return;
			}

			// Создаём метаданные
			MapMetadata metadata;
			metadata.id = QUuid::createUuid();
			metadata.name = mapName;
			metadata.chunkSize.setWidth(dialog.getChunkWidth());
			metadata.chunkSize.setHeight(dialog.getChunkHeight());
			metadata.tileSize = dialog.getTileSize();
			// mapSize оставляем по умолчанию, он будет определяться динамически

			// Сохраняем метаданные
			if (d->mapService->saveMapMetadata(mapName, metadata)) {
				qInfo() << "MapEditorWidget: created new map:" << mapName;

				// Обновляем comboBox и выбираем новую карту
				if (d->mapComboBox) {
					d->mapComboBox->addItem(mapName);
					d->mapComboBox->setCurrentText(mapName);
				}

				// Устанавливаем как текущую карту
				d->mapService->setCurrentMap(mapName);
			}
			else {
				qWarning() << "MapEditorWidget: failed to save map metadata for:" << mapName;
			}
		}
	});

  d->toolbar->addWidget(newMapButton);

  // Кнопка сохранения
  auto* saveButton = new QPushButton("Сохранить");
  connect(saveButton, &QPushButton::clicked, this, &MapEditorWidget::onSaveMap);
  d->toolbar->addWidget(saveButton);
}

void MapEditorWidget::onShowSelectedLayerOnly(bool checked) {
	const auto renderer = tileRenderer();
	if (!renderer) {
		return;
	}

	if (checked) {
		renderer->showOnlyOneLayer(d->tileRenderLayer);
	}
	else {
		renderer->showAllLayers();
	}

	update();
}

void MapEditorWidget::onRenderLevelEditChanged(const QString& text) {
	bool ok = false;
	int value = text.toInt(&ok);
	if (!ok) {
		return;
	}

	setTileRenderLayer(value);
}

void MapEditorWidget::placeTile(int x, int y) {
	if (!d->currentChunk) {
		return;
	}

	const auto chunkSize = d->currentChunk->chunkSize();
	const auto ids = d->tilesSelectorService->getSelectionTiles();

	if (ids.isEmpty()) {
		return;
	}

	const int tilesetWidth = chunkSize.width();
	const int tilesetHeight = chunkSize.height();
	const int baseId = ids.first();
	const int startX = x;
	const int startZ = y;

	for (int i = 0; i < ids.size(); i++) {
		const int tileId = ids[i];
		const int diff = tileId - baseId;
		const int offsetX = diff % tilesetWidth;
		const int offsetZ = diff / tilesetWidth;
		const int worldX = startX + offsetX;
		const int worldZ = startZ + offsetZ;

		//d->currentChunk->setTile(worldX, worldZ, tileId, d->tileRenderLayer);
	}

  emit tilesPlaced(x, y, ids);
  update();
}

void MapEditorWidget::eraseTile(int x, int y) {
	if (!d->currentChunk) {
		return;
	}

	//d->currentChunk->setTile(x, y, -1, d->tileRenderLayer);

  emit tileErased(x, y);
  update();
}

void MapEditorWidget::selectTile(int x, int y) {
	const auto renderer = tileRenderer();
	if (!renderer) {
		return;
	}

	const auto chunk = renderer->getOrCreateChunk(x, y);
	if (!chunk || d->currentChunk == chunk) {
		return;
	}

	if (d->currentChunk) {
		d->currentChunk->setBorderColor(Private::unselectedBorderColor);
		d->currentChunk->setZLevel(0.0f);
	}

	d->currentChunk = chunk;
	d->currentChunk->setBorderColor(Private::selectedBorderColor);
	d->currentChunk->setZLevel(2.0f);
	setZLevel(2.0f);
	update();
}

void MapEditorWidget::onApplySelectedAtlas() {
	ApplyMapAtlasDialog dlg(d->tilesetSettings, this);
	if (dlg.exec() == QDialog::Accepted) {
		d->tilesetSettings = dlg.settings();
		d->needLoadAtlas = true;
		update();
	}
}

void MapEditorWidget::onBeginFrame() {
	if (!d->needLoadAtlas) {
		return;
	}

	d->needLoadAtlas = false;

	if (!d->currentChunk) {
		return;
	}

	if (!d->currentMapMetadata.has_value() || !d->tilesetSettings.has_value() || !d->currentMapName.has_value() || !d->currentChunk) {
		return;
	}

	const auto tileName = d->tilesSelectorService->getTileSetName();
	if (tileName.isEmpty() ) {
		return;
	}

	auto texture = d->textureService->upload(QString("%1.png").arg(tileName),
		d->tilesetSettings.value(), ImageType::TileSets,
		d->currentMapName.value());

	auto atlas = std::make_shared<TextureAtlas>();

	atlas->load(texture,
		d->currentMapMetadata->chunkSize.width(),
		d->currentMapMetadata->chunkSize.height());

	//d->currentChunk->setTileset(atlas);

	updatePropertiesPanel();
}

void MapEditorWidget::setTileRenderLayer(int layer) {
	if (layer >= TileRenderer::kMaxTileRenderLayer || layer < 0) {
		qWarning() << "MapViewBase::setTileRenderLayer:" << layer << ">=" << TileRenderer::kMaxTileRenderLayer << "or < 0";
		return;
	}

	d->tileRenderLayer = layer;
}

int MapEditorWidget::tileRenderLayer() const {
	return d->tileRenderLayer;
}

void MapEditorWidget::setupPropertiesPanel() {
	d->propertiesGroup = new QGroupBox("Свойства");
	auto* layout = new QFormLayout(d->propertiesGroup);

	d->positionLabel = new QLabel("-");
	layout->addRow("Позиция:", d->positionLabel);

	d->tileIdLabel = new QLabel("-");
	layout->addRow("Выбрано тайлов:", d->tileIdLabel);

	d->currentMapLabel = new QLabel("-");
	layout->addRow("Текущая карта:", d->currentMapLabel);

	d->currentAtlasLabel = new QLabel("-");
	layout->addRow("Атлас:", d->currentAtlasLabel);
}

void MapEditorWidget::updatePropertiesPanel() {
	//const auto tilesetSelected = d->currentChunk && d->currentChunk->hasTileset();

	if (d->hoveredTile.has_value()) {
		d->positionLabel->setText(QString("%1, %2")
			.arg(d->hoveredTile->x())
			.arg(d->hoveredTile->y()));
	}
	else {
		d->positionLabel->setText("-");
	}

	const auto ids = d->tilesSelectorService->getSelectionTiles();
	d->tileIdLabel->setText(QString::number(ids.count()));

	const auto currentAtlas = d->tilesSelectorService ?
		d->tilesSelectorService->getTileSetName() :
		QString("Не выбран");

	//if (tilesetSelected) {
	//	d->currentAtlasLabel->setText(currentAtlas);
	//}
	//else {
	//	d->currentAtlasLabel->setText("Не выбран");
	//}

	const auto currentMap = d->mapService ? d->mapService->getCurrentMap() : std::nullopt;
	d->currentMapLabel->setText(currentMap.value_or("Не выбрана"));
}

void MapEditorWidget::onSaveMap() {
	const auto renderer = tileRenderer();
	if (!d->mapService || !renderer) {
		return;
	}

	const auto currentMap = d->mapService->getCurrentMap();
	if (!currentMap.has_value() ) {
		return;
	}

	qInfo() << "Saving map:" << currentMap.value();

	const auto chunkSizes = renderer->chunkSize();
	const auto chunkCount = renderer->chunkCount();

}
