#pragma once
#include "DataFormat/proto/frm.h"
#include "DataFormat/proto/pal.h"
#include <QWidget>
#include <QImage>

class AtlasView : public QWidget {
public:
	AtlasView(QWidget* parent = nullptr);

	void setup(const Proto::Animation& animation, const Proto::Pallete& pallete);

	void setOffset(int x, int y);
	void resetView();
	void fitToView();

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void updateImageRect();
	QPointF mapFromImageToWidget(const QPointF& imagePos) const;
	QPointF mapFromWidgetToImage(const QPointF& widgetPos) const;
	void drawBackgroundLines(QPainter &painter);

private:
	QImage _image;
	QRectF _imageRect; // bounding box изображения в image-space (с учётом offset!)

	// Transform: image → widget
	qreal _scale = 1.0;
	QPointF _pan; // смещение в image-space (целое изображение двигается)

	// Drag state
	bool _isDragging = false;
	QPointF _lastMousePos;
	QPointF _lastPan;
};
