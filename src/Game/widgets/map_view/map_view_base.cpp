#include "Game/widgets/map_view/map_view_base.h"

#include "ApplicationLayer/shaders/shaders_service.h"

#include "Graphics/camera.h"
#include "Graphics/tiles/tile_renderer.h"
#include "Graphics/textures/texture_atlas.h"
#include "Graphics/shaders/shader_program.h"

#include <QOpenGLFunctions_4_5_Core>
#include <QWheelEvent>
#include <memory>

class MapViewBase::Private : public QOpenGLFunctions_4_5_Core {
public:
  Private(MapViewBase* parent) : q(parent) {}

  MapViewBase* q;

  Camera camera;
  bool rightMousePressed = false;
  QPoint lastMousePos;
	float zLevel = 0.0f;

  // Система тайлов
  std::unique_ptr<TileRenderer> tileRenderer;

	ShadersService* shadersService;
	std::shared_ptr<ShaderProgram> shader;
};

MapViewBase::MapViewBase(ShadersService* shadersService, QWidget* parent)
  : d(std::make_unique<Private>(this))
  , QOpenGLWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setAttribute(Qt::WA_NoSystemBackground, true);

  d->tileRenderer = std::make_unique<TileRenderer>();
	d->shadersService = shadersService;
	setDefaultCamera();
}

MapViewBase::~MapViewBase() = default;

void MapViewBase::resetCamera() {
	setDefaultCamera();
  d->camera.update();
  update();
}

void MapViewBase::setDefaultCamera() {
	d->camera.apply(QVector3D(15.0f, 0.0f, 10.0f), QVector3D(15.0f, 45.0f, 25.0f));
}

void MapViewBase::initializeGL() {
  d->initializeOpenGLFunctions();

  emit initializeContext();

	d->glEnable(GL_DEPTH_TEST);
	d->glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
  setupViewport();

  initializeTileSystem();
	initializeShaders();
}

void MapViewBase::initializeTileSystem() {
  // Инициализируем рендерер
  if (!d->tileRenderer->initialize()) {
		qWarning() << "Failed to initialize TileRenderer";
		return;
  }
}

void MapViewBase::initializeShaders() {
	if (!d->shadersService) {
		return;
	}

	d->shader = d->shadersService->loadShader("chunk_base");
	//d->shader->
}

void MapViewBase::resizeGL(int w, int h) {
  setupViewport();
}

void MapViewBase::paintGL() {
  d->glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
  d->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	emit beginFrame();

  if (d->tileRenderer) {
		d->tileRenderer->render(d->camera, width(), height());
  }
}

void MapViewBase::setupViewport() {
  d->glViewport(0, 0, width(), height());
  d->camera.setupViewport(width(), height());
}

void MapViewBase::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
		const auto hit = d->camera.raycastToGround(event->pos(), width(), height(), d->zLevel);
		if (hit) {
			int worldX = int(hit->x());
			int worldZ = int(hit->z());
			if (worldX >= 0 && worldZ >= 0 && worldX < 1024 && worldZ < 1024) {
				std::optional<QPoint> point = QPoint(worldX, worldZ);
				emit tileClicked(point);
			}
		}
		else {
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
  const auto hit = d->camera.raycastToGround(event->pos(), width(), height(), d->zLevel);
  if (hit) {
		int worldX = qRound(hit->x());
		int worldZ = qRound(hit->z());
		if (worldX >= 0 && worldZ >= 0 && worldX < 128 && worldZ < 128) {
			std::optional<QPoint> point = QPoint(worldX, worldZ);
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
  update();
}

TileRenderer* MapViewBase::tileRenderer() const {
  return d->tileRenderer.get();
}

Camera& MapViewBase::camera() {
  return d->camera;
}

const Camera& MapViewBase::camera() const {
  return d->camera;
}

void MapViewBase::setZLevel(float value) {
	d->zLevel = value;
}
