#pragma once

#include <QWidget>
#include <QVariantMap>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QVector3D>
#include <QMatrix4x4>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShader)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class MapView : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
public:
  explicit MapView(QWidget* parent = nullptr);

  virtual ~MapView() = default;

  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

private:
	QMatrix4x4 lookAt(const QVector3D& eye, const QVector3D& center, const QVector3D& up);
	void updateCamera();
	void setupViewport();

private:
	// Камера
		// Камера (look-at)
	QVector3D _cameraEye = QVector3D(5.0f, 8.0f, 5.0f);     // позиция камеры
	QVector3D _cameraCenter = QVector3D(0.0f, 0.0f, 0.0f);  // точка, на которую смотрим
	QVector3D _cameraUp = QVector3D(0.0f, 1.0f, 0.0f);     // направление "вверх"

	bool _rightMousePressed = false;
	QPoint _lastMousePos;

	QMatrix4x4 _projection;
	QMatrix4x4 _view;

	// Шейдер
	QOpenGLShaderProgram _program;

	// Точка
	QOpenGLVertexArrayObject _pointVao;
	QOpenGLBuffer _pointVbo;

	// Сетка
	QOpenGLVertexArrayObject _gridVao;
	QOpenGLBuffer _gridVbo;
	int _gridVertexCount = 0;

	// Оси
	QOpenGLVertexArrayObject _axesVao;
	QOpenGLBuffer _axesVbo;
	int _axesVertexCount = 0;
};
