#include "Graphics/widgets/base_opengl_widget.h"
#include "Graphics/camera.h"
#include <QOpenGLFunctions_4_5_Core>
#include <QWheelEvent>

class BaseOpenGLWidget::Private : public QOpenGLFunctions_4_5_Core {
public:
	Private(BaseOpenGLWidget* parent) : q(parent) {}
	BaseOpenGLWidget* q;

	Camera camera;
	bool rightMousePressed = false;
	QPoint lastMousePos;
	float zLevel = 0.0f;
};


BaseOpenGLWidget::BaseOpenGLWidget(QWidget* parent)
	: QOpenGLWidget(parent)
	, d(std::make_unique<Private>(this)) {
}

BaseOpenGLWidget::~BaseOpenGLWidget() = default;

void BaseOpenGLWidget::resetCamera() {
	setDefaultCamera();
	d->camera.update();
	update();
}

void BaseOpenGLWidget::setDefaultCamera() {
	d->camera.apply(QVector3D(15.0f, 0.0f, 10.0f), QVector3D(15.0f, 45.0f, 25.0f));
}

void BaseOpenGLWidget::initializeGL() {
	d->initializeOpenGLFunctions();

	emit initializeContext();

	d->glEnable(GL_DEPTH_TEST);
	d->glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	setupViewport();
}

void BaseOpenGLWidget::resizeGL(int w, int h) {
	setupViewport();
}

void BaseOpenGLWidget::paintGL() {
	d->glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	d->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	emit beginFrame();
}

void BaseOpenGLWidget::setupViewport() {
	d->glViewport(0, 0, width(), height());
	d->camera.setupViewport(width(), height());
}

void BaseOpenGLWidget::mousePressEvent(QMouseEvent* event) {
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

void BaseOpenGLWidget::mouseMoveEvent(QMouseEvent* event) {
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

void BaseOpenGLWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::RightButton) {
		d->rightMousePressed = false;
	}
}

void BaseOpenGLWidget::wheelEvent(QWheelEvent* event) {
	float zoomFactor = event->angleDelta().y() > 0 ? 0.9f : 1.1f;
	d->camera.zoom(zoomFactor);
	d->camera.update();
	update();
}

Camera& BaseOpenGLWidget::camera() {
	return d->camera;
}

const Camera& BaseOpenGLWidget::camera() const {
	return d->camera;
}

void BaseOpenGLWidget::setZLevel(float value) {
	d->zLevel = value;
}

float BaseOpenGLWidget::zLevel() const {
	return d->zLevel;
}
