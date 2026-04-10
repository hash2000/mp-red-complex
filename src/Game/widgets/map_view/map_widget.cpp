#include "Game/widgets/map_view/map_widget.h"
#include "Game/services/time_service/time_events.h"
#include "Graphics/camera.h"
#include "Graphics/tiles/tile_renderer.h"
#include "Graphics/tiles/tileset.h"
#include "Graphics/textures/texture_atlas.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QVector3D>
#include <QMatrix4x4>
#include <QTimer>
#include <QWheelEvent>
#include <QPixmap>
#include <QPainter>
#include <memory>
#include <list>

class MapWidget::Private {
public:
	Private(MapWidget* parent) : q(parent) { }

	MapWidget* q;

	Camera camera{ QVector3D(5.0f, 0.0f, 5.0f), QVector3D(5.0f, 12.0f, 9.0f) };
	bool rightMousePressed = false;
	QPoint lastMousePos;
	std::optional<QPoint> selectedNode;
	TilesService* tilesService;

	// Система тайлов
	std::unique_ptr<TileRenderer> tileRenderer;
	std::unique_ptr<TextureAtlas> textureAtlas;
	std::unique_ptr<Tileset> tileset;
};

MapWidget::MapWidget(TilesService* tilesService, QWidget* parent)
: d(std::make_unique<Private>(this))
, QOpenGLWidget(parent) {
	setFocusPolicy(Qt::StrongFocus);
	// Устанавливаем атрибуты для избежания мерцания при создании
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_NoSystemBackground, true);

	d->tilesService = tilesService;
	d->tileRenderer = std::make_unique<TileRenderer>();
	d->textureAtlas = std::make_unique<TextureAtlas>(32, 32); // Тайлы 32x32
	d->tileset = std::make_unique<Tileset>();
}

MapWidget::~MapWidget() = default;

void MapWidget::initializeGL() {
	initializeOpenGLFunctions();

	emit initializeContext();

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	setupViewport();

	// Инициализируем систему тайлов
	initializeTileSystem();
}

void MapWidget::initializeTileSystem() {
	// Инициализируем рендерер
	if (!d->tileRenderer->initialize()) {
		qWarning() << "Failed to initialize TileRenderer";
		return;
	}

	// Создаем тестовый тайлсет (заглушку)
	// В реальном проекте здесь будет загрузка из файла
	QPixmap testAtlas(256, 256);
	testAtlas.fill(Qt::gray);

	// Рисуем несколько цветных квадратов для тестирования
	QPainter painter(&testAtlas);
	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			QColor color = QColor::fromHsv((x + y) * 20, 150, 200);
			painter.fillRect(x * 32, y * 32, 32, 32, color);
		}
	}
	painter.end();

	// Загружаем атлас
	if (d->textureAtlas->loadFromPixmap(testAtlas, 8, 8)) {
		qInfo() << "Test texture atlas loaded successfully";

		// Инициализируем тайлсет
		d->tileset->initialize(d->textureAtlas.get(), 8, 8);
		d->tileRenderer->setTileset(d->tileset.get());

		// Создаем тестовые чанки (4 чанка 16x16 = 64x64 тайла)
		for (int cz = 0; cz < 2; cz++) {
			for (int cx = 0; cx < 2; cx++) {
				auto* chunk = d->tileRenderer->getOrCreateChunk(cx, cz);

				// Заполняем тестовыми тайлами
				for (int z = 0; z < 16; z++) {
					for (int x = 0; x < 16; x++) {
						// Чередуем тайлы
						int tileId = (x + z) % 16;
						chunk->setTile(x, z, tileId);
					}
				}
			}
		}

		qInfo() << "Created" << d->tileRenderer->chunkCount() << "chunks";
	}
}

void MapWidget::resizeGL(int w, int h) {
	setupViewport();
}

void MapWidget::paintGL() {
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Рендерим тайлы
	if (d->tileRenderer) {
		d->tileRenderer->render(d->camera, width(), height());
	}

	emit paintView();
}

void MapWidget::setupViewport() {
	glViewport(0, 0, width(), height());
	d->camera.setupViewport(width(), height());
}

void MapWidget::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		const auto hit = d->camera.raycastToGround(event->pos(), width(), height());
		if (hit) {
			int worldX = qRound(hit->x());
			int worldZ = qRound(hit->z());
			// Проверяем границы (например, 0..127)
			if (worldX >= 0 && worldZ >= 0 && worldX < 128 && worldZ < 128) {
				d->selectedNode = QPoint(worldX, worldZ);
				emit selectNode(d->selectedNode);
				qDebug() << "Selected:" << worldX << worldZ;
			}
		}
		else {
			emit selectNode(std::nullopt);
		}
	}
	else if (event->button() == Qt::RightButton) {
		d->rightMousePressed = true;
		d->lastMousePos = event->pos();
	}
}

void MapWidget::mouseMoveEvent(QMouseEvent* event) {
	if (d->selectedNode) {
		QPoint delta = event->pos() - d->lastMousePos;
		//float newH = m_terrain.height(m_selectedNode->x(), m_selectedNode->y()) + delta;
		//m_terrain.setHeight(m_selectedNode->x(), m_selectedNode->y(), newH);
		//update(); // чанки сами перестроятся при рендере
	}
	else if (d->rightMousePressed) {
		QPoint delta = event->pos() - d->lastMousePos;
		d->lastMousePos = event->pos();
		d->camera.move(delta);
		d->camera.update();
		update();
	}
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::RightButton) {
		d->rightMousePressed = false;
	}

	if (event->button() == Qt::LeftButton) {
		d->selectedNode = std::nullopt;
	}
}

void MapWidget::wheelEvent(QWheelEvent* event) {
	float zoomFactor = event->angleDelta().y() > 0 ? 0.9f : 1.1f;
	d->camera.zoom(zoomFactor);
	d->camera.update();
	update();
}

void MapWidget::onTick(const TickEvent& event) {
	update();
}
