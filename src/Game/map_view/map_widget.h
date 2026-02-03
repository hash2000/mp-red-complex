#pragma once
#include "Game/event_bus/events/time_events.h"
#include <QVariantMap>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>

class MapWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
	Q_OBJECT
public:
	MapWidget(QWidget* parent = nullptr);
	~MapWidget() override;

	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;	

public slots:
	void onTick(const TickEvent& event);

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
