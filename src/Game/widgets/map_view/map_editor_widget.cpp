#include "Game/widgets/map_view/map_editor_widget.h"
#include "Game/widgets/map_view/create_map_dialog.h"
#include "Game/widgets/map_view/apply_map_filter_dialog.h"
#include "ApplicationLayer/maps/map_service.h"
#include "ApplicationLayer/textures/tiles_service.h"
#include "Graphics/textures/texture_atlas.h"
#include "Graphics/tiles/tileset.h"
#include "Graphics/tiles/tile_renderer.h"
#include "Graphics/tiles/chunk.h"
#include <QToolBar>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDebug>
#include <QPainter>

class MapEditorWidget::Private {
public:
  Private(MapEditorWidget* parent) : q(parent) {}

  MapEditorWidget* q;
  MapService* mapService = nullptr;
  TilesService* tilesService = nullptr;

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
  MapEditorMode currentMode = MapEditorMode::Draw;
  QList<int> selectedTileIds;
  std::optional<QPoint> hoveredTile;
  bool isDrawing = false;
	bool needLoadAtlas = false;
};

MapEditorWidget::MapEditorWidget(
  MapService* mapService,
  TilesService* tilesService,
  QWidget* parent)
  : MapViewBase(parent)
  , d(std::make_unique<Private>(this)) {
  d->mapService = mapService;
  d->tilesService = tilesService;

  setMapService(mapService);
  setTilesService(tilesService);

  setupUI();

  // Подписываемся на изменения карты
  if (d->mapService) {
		connect(d->mapService, &MapService::currentMapChanged,
			this, &MapEditorWidget::onMapChanged);
  }

	connect(this, &MapViewBase::beginFrame, this, &MapEditorWidget::onBeginFrame);
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
  d->currentMode = static_cast<MapEditorMode>(index);
  emit modeChanged(d->currentMode);
  qDebug() << "Map editor mode changed to:" << index;
}

void MapEditorWidget::onTileSelected(const QList<int>& tileIds) {
  d->selectedTileIds = tileIds;
  updatePropertiesPanel();
  qDebug() << "Tile selected for drawing:" << tileIds.count();
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
		if (d->selectedTileIds.count() >= 0) {
			placeTile(point->x(), point->y());
		}
		break;
  case MapEditorMode::Erase:
		eraseTile(point->x(), point->y());
		break;
  case MapEditorMode::Select:
		// TODO: Реализовать выделение области
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
  d->modeComboBox->addItem("Рисование");
  d->modeComboBox->addItem("Стирание");
  d->modeComboBox->addItem("Выделение");
  d->modeComboBox->addItem("Свойства");
  connect(d->modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &MapEditorWidget::onModeChanged);

  d->toolbar->addWidget(d->modeComboBox);

	auto* applyPixmap = new QPushButton("> атлас");
	connect(applyPixmap, &QPushButton::clicked, this, &MapEditorWidget::onApplySelectedAtlas);

	d->toolbar->addWidget(applyPixmap);

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
  connect(saveButton, &QPushButton::clicked, this, [this]() {
		if (d->mapService) {
			const auto currentMap = d->mapService->getCurrentMap();
			if (currentMap.has_value()) {
				// TODO: Сохранить изменения
				qInfo() << "Saving map:" << currentMap.value();
			}
		}
	});
  d->toolbar->addWidget(saveButton);
}

void MapEditorWidget::setupPropertiesPanel() {
  d->propertiesGroup = new QGroupBox("Свойства");
  auto* layout = new QFormLayout(d->propertiesGroup);

  d->positionLabel = new QLabel("-");
  layout->addRow("Позиция:", d->positionLabel);

  d->tileIdLabel = new QLabel("-");
  layout->addRow("Тайл для рисования:", d->tileIdLabel);

  d->currentMapLabel = new QLabel("-");
  layout->addRow("Текущая карта:", d->currentMapLabel);

  d->currentAtlasLabel = new QLabel("-");
  layout->addRow("Атлас:", d->currentAtlasLabel);
}

void MapEditorWidget::placeTile(int x, int y) {
	const auto tiles = tilesService();
	const auto renderer = tileRenderer();
	if (!renderer) {
		return;
	}

	const auto chunkSizes = renderer->chunkSize();
	const auto ids = tiles->getSelectionTiles();

	if (ids.isEmpty()) {
		return;
	}

	const int tilesetWidth = chunkSizes.width();
	const int tilesetHeight = chunkSizes.height();
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

		auto chunk = renderer->getOrCreateChunk(worldX, worldZ);
		if (!chunk) {
			return;
		}

		chunk->setTile(worldX, worldZ, tileId);
	}

  emit tilesPlaced(x, y, d->selectedTileIds);
	updatePropertiesPanel();
  update();
}

void MapEditorWidget::eraseTile(int x, int y) {
  // TODO: Реализовать стирание тайла через MapService
  qDebug() << "Erasing tile at" << x << y;
  emit tileErased(x, y);
  update();
}

void MapEditorWidget::updatePropertiesPanel() {
  if (d->hoveredTile.has_value()) {
		d->positionLabel->setText(QString("%1, %2").arg(d->hoveredTile->x()).arg(d->hoveredTile->y()));
  }
  else {
		d->positionLabel->setText("-");
  }

  if (d->selectedTileIds.count() >= 0) {
		d->tileIdLabel->setText(QString::number(d->selectedTileIds.count()));
  }
  else {
		d->tileIdLabel->setText("Не выбран");
  }

	const auto currentAtlas = d->tilesService ? d->tilesService->getTileSetName() : QString("Не выбран");
	d->currentAtlasLabel->setText(currentAtlas);

  const auto currentMap = d->mapService ? d->mapService->getCurrentMap() : std::nullopt;
  d->currentMapLabel->setText(currentMap.value_or("Не выбрана"));
}

void MapEditorWidget::onTileServiceConnected() {
  auto service = tilesService();
  connect(service, &TilesService::tilesSelectionChanged, this, &MapEditorWidget::onTileSelected);
}

void MapEditorWidget::onApplySelectedAtlas() {
	ApplyMapAtlasDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted) {
		auto settings = dlg.settings();

		d->needLoadAtlas = true;
		update();
	}

}

void MapEditorWidget::onBeginFrame() {
	if (d->needLoadAtlas) {
		d->needLoadAtlas = false;
		loadTilemap();
	}
}
