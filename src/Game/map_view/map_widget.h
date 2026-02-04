#pragma once
#include "Game/services/time_service/time_events.h"
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

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

private:
	void setupViewport();

public slots:
	void onTick(const TickEvent& event);

signals:
	void initializeContext();
	void paintView();
	void selectNode(std::optional<QPoint> point);

private:
	class Private;
	std::unique_ptr<Private> d;
};
