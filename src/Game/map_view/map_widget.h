#pragma once
#include <QVariantMap>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>

class MapDocument;

class MapWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
public:
	MapWidget(QWidget* parent = nullptr);

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
	class Private;
	std::unique_ptr<Private> d;
};
