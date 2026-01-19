#include "Content/MapEditor/map_view.h"
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
	, _pointVbo(QOpenGLBuffer::VertexBuffer)
	, _gridVbo(QOpenGLBuffer::VertexBuffer)
	, _axesVbo(QOpenGLBuffer::VertexBuffer)
{
	setFocusPolicy(Qt::StrongFocus);
}

void MapView::initializeGL()
{
	initializeOpenGLFunctions();

	// Шейдеры
	if (!_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource))
		qWarning() << "Vertex shader failed:" << _program.log();
	if (!_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource))
		qWarning() << "Fragment shader failed:" << _program.log();
	if (!_program.link())
		qFatal("Shader linking failed: %s", qPrintable(_program.log()));

	// === Точка (оставим для отладки) ===
	QVector<float> pointData = { 0.0f, 0.0f, 0.0f };
	_pointVbo.create();
	_pointVbo.bind();
	_pointVbo.allocate(pointData.data(), static_cast<int>(pointData.size() * sizeof(float)));
	_pointVbo.release();

	_pointVao.create();
	_pointVao.bind();
	_program.bind();
	_pointVbo.bind();
	_program.enableAttributeArray("aPos");
	_program.setAttributeBuffer("aPos", GL_FLOAT, 0, 3, 0);
	_pointVao.release();

	// === Сетка 10x10 на XZ (Y=0) ===
	QVector<float> gridVerts;
	for (int i = 0; i <= 10; i++)
	{
		float f = static_cast<float>(i);
		// Линии вдоль Z (по X)
		gridVerts << 0.0f << 0.0f << f;
		gridVerts << 10.0f << 0.0f << f;
		// Линии вдоль X (по Z)
		gridVerts << f << 0.0f << 0.0f;
		gridVerts << f << 0.0f << 10.0f;
	}
	_gridVertexCount = gridVerts.size() / 3;

	_gridVbo.create();
	_gridVbo.bind();
	_gridVbo.allocate(gridVerts.data(), static_cast<int>(gridVerts.size() * sizeof(float)));
	_gridVbo.release();

	_gridVao.create();
	_gridVao.bind();
	_program.bind();
	_gridVbo.bind();
	_program.enableAttributeArray("aPos");
	_program.setAttributeBuffer("aPos", GL_FLOAT, 0, 3, 0);
	_gridVao.release();

	// === Оси: X (красная), Z (синяя) ===
	QVector<float> axesVerts = {
		// X axis
		0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Y axis
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Z axis
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	};
	_axesVertexCount = axesVerts.size() / 3;

	_axesVbo.create();
	_axesVbo.bind();
	_axesVbo.allocate(axesVerts.data(), static_cast<int>(axesVerts.size() * sizeof(float)));
	_axesVbo.release();

	_axesVao.create();
	_axesVao.bind();
	_program.bind();
	_axesVbo.bind();
	_program.enableAttributeArray("aPos");
	_program.setAttributeBuffer("aPos", GL_FLOAT, 0, 3, 0);
	_axesVao.release();

	_program.release();

	// OpenGL state
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

	// Камера: смотрим на центр сетки (5,0,5)
	_cameraCenter = QVector3D(5.0f, 0.0f, 5.0f);
	_cameraEye = QVector3D(5.0f, 12.0f, 12.0f); // вид под углом

	setupViewport();
}

void MapView::resizeGL(int w, int h)
{
	setupViewport();
}

// === Вспомогательная функция lookAt ===
QMatrix4x4 MapView::lookAt(const QVector3D& eye, const QVector3D& center, const QVector3D& up)
{
	QMatrix4x4 view;
	QVector3D f = (center - eye).normalized();
	QVector3D s = QVector3D::crossProduct(f, up).normalized();
	QVector3D u = QVector3D::crossProduct(s, f);

	view(0, 0) = s.x();  view(0, 1) = s.y();  view(0, 2) = s.z();
	view(1, 0) = u.x();  view(1, 1) = u.y();  view(1, 2) = u.z();
	view(2, 0) = -f.x(); view(2, 1) = -f.y(); view(2, 2) = -f.z();

	view(0, 3) = -QVector3D::dotProduct(s, eye);
	view(1, 3) = -QVector3D::dotProduct(u, eye);
	view(2, 3) = QVector3D::dotProduct(f, eye);

	return view;
}

void MapView::setupViewport()
{
	glViewport(0, 0, width(), height());
	float aspect = static_cast<float>(width()) / height();
	_projection.setToIdentity();
	_projection.perspective(45.0f, aspect, 0.1f, 100.0f);
	updateCamera();
}

void MapView::updateCamera()
{
	_view = lookAt(_cameraEye, _cameraCenter, _cameraUp);
}

void MapView::paintGL()
{
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!_program.isLinked()) {
		return;
	}

	_program.bind();
	_program.setUniformValue("uProjection", _projection);
	_program.setUniformValue("uView", _view);

	// Сетка
	_program.setUniformValue("uColor", QVector3D(0.4f, 0.4f, 0.4f));
	_gridVao.bind();
	glDrawArrays(GL_LINES, 0, _gridVertexCount);
	_gridVao.release();

	// Оси
	_axesVao.bind();

	glLineWidth(2.0f);

	// X — красная
	_program.setUniformValue("uColor", QVector3D(1.0f, 0.0f, 0.0f));
	glDrawArrays(GL_LINES, 0, 2);

	// Y — зелёная
	_program.setUniformValue("uColor", QVector3D(0.0f, 1.0f, 0.0f));
	glDrawArrays(GL_LINES, 2, 2);

	// Z — синяя
	_program.setUniformValue("uColor", QVector3D(0.0f, 0.0f, 1.0f));
	glDrawArrays(GL_LINES, 4, 2);

	glLineWidth(1.0f);

	_axesVao.release();

	// Точка (опционально, для отладки)
	// m_program.setUniformValue("uColor", QVector3D(1.0f, 1.0f, 0.0f));
	// m_pointVao.bind();
	// glDrawArrays(GL_POINTS, 0, 1);
	// m_pointVao.release();

	_program.release();
}

// === Управление мышью ===

void MapView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::RightButton) {
		_rightMousePressed = true;
		_lastMousePos = event->pos();
	}
}

void MapView::mouseMoveEvent(QMouseEvent* event)
{
	if (_rightMousePressed) {
		QPoint delta = event->pos() - _lastMousePos;
		_lastMousePos = event->pos();

		float speed = 0.01f;

		// Получаем направление "вперёд" и "вправо" в мировой системе координат
		// На основе текущего взгляда камеры, но проигнорируем Y
		QVector3D viewDir = (_cameraCenter - _cameraEye).normalized();
		viewDir.setY(0.0f);
		if (viewDir.length() < 1e-6f) {
			viewDir = QVector3D(0.0f, 0.0f, -1.0f); // fallback
		}
		viewDir.normalize();

		QVector3D rightDir = QVector3D::crossProduct(viewDir, QVector3D(0.0f, 1.0f, 0.0f)).normalized();

		// Вектор смещения в мировых координатах
		QVector3D panOffset = -rightDir * static_cast<float>(delta.x()) * speed
			+ viewDir * static_cast<float>(delta.y()) * speed;

		// Двигаем и глаз, и центр ОДИНАКОВО
		_cameraEye += panOffset;
		_cameraCenter += panOffset;

		updateCamera();
		update();
	}
}

void MapView::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::RightButton) {
		_rightMousePressed = false;
	}
}

void MapView::wheelEvent(QWheelEvent* event)
{
	float zoomFactor = event->angleDelta().y() > 0 ? 0.9f : 1.1f; // масштаб ближе/дальше

	// Вектор от глаза к центру
	QVector3D viewDir = _cameraCenter - _cameraEye;
	float currentDistance = viewDir.length();

	// Минимальное и максимальное расстояние
	float minDistance = 2.0f;
	float maxDistance = 50.0f;

	float newDistance = currentDistance * zoomFactor;
	newDistance = qBound(minDistance, newDistance, maxDistance);

	// Новый глаз = центр - нормализованный вектор * новое расстояние
	viewDir.normalize();
	_cameraEye = _cameraCenter - viewDir * newDistance;

	updateCamera();
	update();
}
