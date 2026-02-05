#include "Content/MapEditor/map_view.h"
#include "Content/MapEditor/DrawBuffers/once_point_buffer.h"
#include "Content/MapEditor/DrawBuffers/grid_buffer.h"
#include "Content/MapEditor/DrawBuffers/axes_buffer.h"
#include "Content/MapEditor/DrawBuffers/terrain_chunk_buffer.h"
#include <QMouseEvent>
#include <QOpenGLShader>

static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uProjection;
uniform mat4 uView;
void main()
{
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main()
{
    FragColor = vec4(uColor, 1.0);
}
)";

MapView::MapView(QWidget* parent)
	: QOpenGLWidget(parent)
{
	setFocusPolicy(Qt::StrongFocus);
}

void MapView::initializeGL()
{
	initializeOpenGLFunctions();

	_program.create(vertexShaderSource, fragmentShaderSource);	
//	_program.add(std::make_unique<OncePointDrawBuffer>());
//	_program.add(std::make_unique<GridDrawBuffer>());
	_program.add(std::make_unique<AxesDrawBuffer>());
	_program.add(std::make_unique<TerrainChunkDrawBuffer>(2, 2));
	_program.release();

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

	setupViewport();
}

void MapView::resizeGL(int w, int h)
{
	setupViewport();
}

void MapView::setupViewport()
{
	glViewport(0, 0, width(), height());
	_camera.setupViewport(width(), height());
}

void MapView::paintGL()
{
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!_program.validate()) {
		return;
	}

	_program.bind();
	_program.setUniformValue("uProjection", _camera.projection());
	_program.setUniformValue("uView", _camera.view());
	_program.render();
}

void MapView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		const auto hit = _camera.raycastToGround(event->pos(), width(), height());
		if (hit) {
			int worldX = qRound(hit->x());
			int worldZ = qRound(hit->z());
			// Проверяем границы (например, 0..127)
			if (worldX >= 0 && worldZ >= 0 && worldX < 128 && worldZ < 128) {
				_selectedNode = QPoint(worldX, worldZ);
				qDebug() << "Selected:" << worldX << worldZ;
			}
		}
	}
	else if (event->button() == Qt::RightButton) {
		_rightMousePressed = true;
		_lastMousePos = event->pos();
	}
}

void MapView::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::RightButton) {
		_rightMousePressed = false;
	}

	if (event->button() == Qt::LeftButton) {
		_selectedNode = std::nullopt;
	}
}

void MapView::mouseMoveEvent(QMouseEvent* event)
{
	if (_selectedNode) {
		QPoint delta = event->pos() - _lastMousePos;
		//float newH = m_terrain.height(m_selectedNode->x(), m_selectedNode->y()) + delta;
		//m_terrain.setHeight(m_selectedNode->x(), m_selectedNode->y(), newH);
		//update(); // чанки сами перестроятся при рендере
	}
	else if (_rightMousePressed) {
		QPoint delta = event->pos() - _lastMousePos;
		_lastMousePos = event->pos();
		_camera.move(delta);
		_camera.update();
		update();
	}
}

void MapView::wheelEvent(QWheelEvent* event)
{
	float zoomFactor = event->angleDelta().y() > 0 ? 0.9f : 1.1f;
	_camera.zoom(zoomFactor);
	_camera.update();
	update();
}
