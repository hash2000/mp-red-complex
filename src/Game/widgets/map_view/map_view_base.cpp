#include "Game/widgets/map_view/map_view_base.h"
#include "Graphics/camera.h"
#include "Graphics/tiles/tile_renderer.h"
#include "Graphics/tiles/tileset.h"
#include "Graphics/textures/texture_atlas.h"
#include "ApplicationLayer/textures/tiles_service.h"
#include "ApplicationLayer/maps/map_service.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QVector3D>
#include <QMatrix4x4>
#include <QWheelEvent>
#include <QPixmap>
#include <QPainter>
#include <memory>

class MapViewBase::Private {
public:
  Private(MapViewBase* parent) : q(parent) {}

  MapViewBase* q;

  Camera camera;
  bool rightMousePressed = false;
  QPoint lastMousePos;

  // Сервисы
  TilesService* tilesService = nullptr;
  MapService* mapService = nullptr;

  // Система тайлов
  std::unique_ptr<TileRenderer> tileRenderer;
  std::unique_ptr<TextureAtlas> textureAtlas;
  std::unique_ptr<Tileset> tileset;
};

MapViewBase::MapViewBase(QWidget* parent)
  : d(std::make_unique<Private>(this))
  , QOpenGLWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setAttribute(Qt::WA_NoSystemBackground, true);

  d->tileRenderer = std::make_unique<TileRenderer>();
  d->textureAtlas = std::make_unique<TextureAtlas>(32, 32);
  d->tileset = std::make_unique<Tileset>();
	setDefaultCamera();
}

MapViewBase::~MapViewBase() = default;

void MapViewBase::setTilesService(TilesService* tilesService) {
  d->tilesService = tilesService;
  onTileServiceConnected();
}

void MapViewBase::setMapService(MapService* mapService) {
  d->mapService = mapService;
  onMapServiceConnected();
}

void MapViewBase::resetCamera() {
	setDefaultCamera();
  d->camera.update();
  update();
}

void MapViewBase::setDefaultCamera() {
	d->camera.apply(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 12.0f, 4.0f));
}

void MapViewBase::loadTilemap() {
  // Получаем QPixmap через TilesService
  if (!d->tilesService) {
		qWarning() << "MapViewBase::loadTilemap: tilesService is null";
		return;
  }

	const auto metadata = d->tilesService->getCurrentTileSetMetadata();
	if (!metadata.has_value()) {
		qWarning() << "MapViewBase::loadTilemap: no tileset metadata";
		return;
	}

	const auto tilesCountX = metadata->gridSize.x;
	const auto tilesCountY = metadata->gridSize.y;
	auto pixmap = d->tilesService->getTilemap();
	if (!pixmap.has_value() || pixmap->isNull()) {
		qWarning() << "MapViewBase::loadTilemap: pixmap is null";
		return;
	}

	qInfo() << "MapViewBase::loadTilemap: loading pixmap" 
		<< pixmap->width() << "x" << pixmap->height();

	// Загружаем в TextureAtlas
	if (d->textureAtlas->loadFromPixmap(pixmap.value(), tilesCountX, tilesCountY)) {
		qInfo() << "Tilemap loaded successfully from TilesService";
		qInfo() << "  - atlas texture ID:" << d->textureAtlas->textureId();
		qInfo() << "  - tiles:" << tilesCountX << "x" << tilesCountY;

		d->tileset->initialize(d->textureAtlas.get(), tilesCountX, tilesCountY);
		qInfo() << "  - tileset initialized, atlas:" << d->tileset->atlas();
		
		d->tileRenderer->setTileset(d->tileset.get());
		qInfo() << "  - tileset set to renderer";

		update();
	} else {
		qWarning() << "MapViewBase::loadTilemap: failed to load pixmap into atlas";
	}
}

void MapViewBase::initializeGL() {
  initializeOpenGLFunctions();

  emit initializeContext();

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
  setupViewport();

  initializeTileSystem();
}

void MapViewBase::initializeTileSystem() {
  // Инициализируем рендерер
  if (!d->tileRenderer->initialize()) {
		qWarning() << "Failed to initialize TileRenderer";
		return;
  }
}

void MapViewBase::resizeGL(int w, int h) {
  setupViewport();
}

void MapViewBase::paintGL() {
  glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	emit beginFrame();

  if (d->tileRenderer) {
		d->tileRenderer->render(d->camera, width(), height());
  }

}

void MapViewBase::setupViewport() {
  glViewport(0, 0, width(), height());
  d->camera.setupViewport(width(), height());
}

void MapViewBase::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
		const auto hit = d->camera.raycastToGround(event->pos(), width(), height());
		if (hit) {
			int worldX = int(hit->x());
			int worldZ = int(hit->z());
			if (worldX >= 0 && worldZ >= 0 && worldX < 1024 && worldZ < 1024) {
				std::optional<QPoint> point = QPoint(worldX, worldZ);
				onTileClicked(point);
				emit tileClicked(point);
			}
		}
		else {
			onTileClicked(std::nullopt);
			emit tileClicked(std::nullopt);
		}
  }
  else if (event->button() == Qt::RightButton) {
		d->rightMousePressed = true;
		d->lastMousePos = event->pos();
  }
}

void MapViewBase::mouseMoveEvent(QMouseEvent* event) {
  // Обновляем hover
  const auto hit = d->camera.raycastToGround(event->pos(), width(), height());
  if (hit) {
		int worldX = qRound(hit->x());
		int worldZ = qRound(hit->z());
		if (worldX >= 0 && worldZ >= 0 && worldX < 128 && worldZ < 128) {
			std::optional<QPoint> point = QPoint(worldX, worldZ);
			onTileHovered(point);
			emit tileHovered(point);
		}
  }

  // Правая кнопка — движение камеры
  if (d->rightMousePressed) {
		const bool ctrlPressed = (event->modifiers() & Qt::ControlModifier);
		QPoint delta = event->pos() - d->lastMousePos;
		d->lastMousePos = event->pos();

		if (ctrlPressed) {
			d->camera.rotate(delta.x(), delta.y());
		}
		else {
			d->camera.move(delta);
		}

		// d->camera.update();
		onRightMouseDrag(delta);
		update();
  }
}

void MapViewBase::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::RightButton) {
		d->rightMousePressed = false;
  }
}

void MapViewBase::wheelEvent(QWheelEvent* event) {
  float zoomFactor = event->angleDelta().y() > 0 ? 0.9f : 1.1f;
  d->camera.zoom(zoomFactor);
  d->camera.update();
  onZoom(zoomFactor);
  update();
}

void MapViewBase::onTileClicked(std::optional<QPoint> point) {
  // Базовая реализация — ничего не делает
  Q_UNUSED(point);
}

void MapViewBase::onTileServiceConnected() {

}

void MapViewBase::onTileHovered(std::optional<QPoint> point) {
  // Базовая реализация — ничего не делает
  Q_UNUSED(point);
}

void MapViewBase::onMapServiceConnected() {

}

void MapViewBase::onRightMouseDrag(QPoint delta) {
  // Базовая реализация — ничего не делает
  Q_UNUSED(delta);
}

void MapViewBase::onZoom(float zoomFactor) {
  // Базовая реализация — ничего не делает
  Q_UNUSED(zoomFactor);
}

TileRenderer* MapViewBase::tileRenderer() const {
  return d->tileRenderer.get();
}

TextureAtlas* MapViewBase::textureAtlas() const {
  return d->textureAtlas.get();
}

Tileset* MapViewBase::tileset() const {
  return d->tileset.get();
}

TilesService* MapViewBase::tilesService() const {
  return d->tilesService;
}

MapService* MapViewBase::mapService() const {
  return d->mapService;
}

Camera& MapViewBase::camera() {
  return d->camera;
}

const Camera& MapViewBase::camera() const {
  return d->camera;
}
