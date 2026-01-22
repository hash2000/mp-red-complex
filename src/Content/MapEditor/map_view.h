#pragma once
#include "Content/Shared/camera.h"
#include "Content/Shared/Draw/draw_program.h"
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
	void setupViewport();

private:
	Camera _camera{ QVector3D(5.0f, 0.0f, 5.0f), QVector3D(5.0f, 12.0f, 12.0f) };

	bool _rightMousePressed = false;
	QPoint _lastMousePos;

	DrawProgram _program;

	std::optional<QPoint> _selectedNode;
};
