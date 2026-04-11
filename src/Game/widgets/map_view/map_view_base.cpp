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

    Camera camera{ QVector3D(5.0f, 0.0f, 5.0f), QVector3D(5.0f, 12.0f, 9.0f) };
    bool rightMousePressed = false;
    QPoint lastMousePos;

    // Сервисы
    TilesService* tilesService = nullptr;
    MapService* mapService = nullptr;

    // Система тайлов
    std::unique_ptr<TileRenderer> tileRenderer;
    std::unique_ptr<TextureAtlas> textureAtlas;
    std::unique_ptr<Tileset> tileset;

		void generateTestAtlas() {
			// Создаём тестовый тайлсет (заглушку)
			QPixmap testAtlas(256, 256);
			testAtlas.fill(Qt::gray);

			QPainter painter(&testAtlas);
			for (int y = 0; y < 8; y++) {
				for (int x = 0; x < 8; x++) {
					QColor color = QColor::fromHsv((x + y) * 20, 150, 200);
					painter.fillRect(x * 32, y * 32, 32, 32, color);
				}
			}
			painter.end();

			// Загружаем атлас
			if (textureAtlas->loadFromPixmap(testAtlas, 8, 8)) {
				qInfo() << "Test texture atlas loaded successfully";

				tileset->initialize(textureAtlas.get(), 8, 8);
				tileRenderer->setTileset(tileset.get());

				// Создаём тестовые чанки (2x2 = 32x32 тайла)
				for (int cz = 0; cz < 2; cz++) {
					for (int cx = 0; cx < 2; cx++) {
						auto* chunk = tileRenderer->getOrCreateChunk(cx, cz);

						for (int z = 0; z < 16; z++) {
							for (int x = 0; x < 16; x++) {
								int tileId = (x + z) % 16;
								chunk->setTile(x, z, tileId);
							}
						}
					}
				}

				qInfo() << "Created" << tileRenderer->chunkCount() << "chunks";
			}
		}
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
    d->camera = Camera{ QVector3D(5.0f, 0.0f, 5.0f), QVector3D(5.0f, 12.0f, 9.0f) };
    d->camera.update();
    update();
}

void MapViewBase::loadTilemap(const QString& tileTexturePath) {
    // Получаем QPixmap через TilesService
    if (d->tilesService) {
        auto pixmap = d->tilesService->getTilemap();
        if (pixmap.has_value() && !pixmap->isNull()) {
            // Загружаем в TextureAtlas
            if (d->textureAtlas->loadFromPixmap(*pixmap, 8, 8)) {
                qInfo() << "Tilemap loaded successfully from TilesService";
                d->tileset->initialize(d->textureAtlas.get(), 8, 8);
                d->tileRenderer->setTileset(d->tileset.get());
                update();
            }
        }
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

  if (d->tileRenderer) {
		d->tileRenderer->render(d->camera, width(), height());
	}

	emit paintView();
}

void MapViewBase::setupViewport() {
    glViewport(0, 0, width(), height());
    d->camera.setupViewport(width(), height());
}

void MapViewBase::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        const auto hit = d->camera.raycastToGround(event->pos(), width(), height());
        if (hit) {
            int worldX = qRound(hit->x());
            int worldZ = qRound(hit->z());
            if (worldX >= 0 && worldZ >= 0 && worldX < 128 && worldZ < 128) {
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
        QPoint delta = event->pos() - d->lastMousePos;
        d->lastMousePos = event->pos();
        d->camera.move(delta);
        d->camera.update();
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
