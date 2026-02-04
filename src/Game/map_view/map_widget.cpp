#include "Game/map_view/map_widget.h"
#include "Game/services/time_service/time_events.h"
#include "Content/Shared/camera.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QVector3D>
#include <QMatrix4x4>
#include <QTimer>
#include <QWheelEvent>
#include <memory>
#include <list>

class MapWidget::Private {
public:
	Private(MapWidget* parent) : q(parent) {

	}

	MapWidget* q;

	Camera camera{ QVector3D(5.0f, 0.0f, 5.0f), QVector3D(5.0f, 12.0f, 12.0f) };
	bool rightMousePressed = false;
	QPoint lastMousePos;
	std::optional<QPoint> selectedNode;
};


MapWidget::MapWidget(QWidget* parent)
: d(std::make_unique<Private>(this))
, QOpenGLWidget(parent) {
	setFocusPolicy(Qt::StrongFocus);
}

MapWidget::~MapWidget() = default;

void MapWidget::initializeGL() {
	initializeOpenGLFunctions();

	emit initializeContext();

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	setupViewport();
}

void MapWidget::resizeGL(int w, int h) {
	setupViewport();
}

void MapWidget::paintGL() {
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
