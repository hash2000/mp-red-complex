#pragma once
#include "DataFormat/proto/frm.h"
#include "DataFormat/proto/pal.h"
#include <QWidget>
#include <QImage>
#include <memory>

class AtlasView : public QWidget {
public:
	AtlasView(QWidget* parent = nullptr);

	void setup(
		std::shared_ptr<const Proto::Animation> animation,
		std::shared_ptr<const Proto::Pallete> pallete);

	void setDirection(size_t direction);
	void showRuler(bool show);

	void setOffset(int x, int y);
	void resetView();
	void fitToView();

	size_t directionsCount() const;
	size_t animationsCount(int direction) const;
	const Proto::Animation* animation() const noexcept;
	const Proto::Direction& direction(size_t idx) const;
	const Proto::Frame& frame(size_t dirIdx, size_t frameIdx) const;

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void setupFrames();
	void updateImageRect();
	QPointF mapFromImageToWidget(const QPointF& imagePos) const;
	QPointF mapFromWidgetToImage(const QPointF& widgetPos) const;
	void drawBackgroundRulerLines(QPainter &painter);

private:
	std::shared_ptr<const Proto::Animation> _animation;
	std::shared_ptr<const Proto::Pallete> _pallete;

	bool _showRuller{ true };
	size_t _direction{ 0 };


	QImage _image;
	QRectF _imageRect; // bounding box изображения в image-space (с учётом offset!)

	// Transform: image → widget
	qreal _scale = 1.0;
	QPointF _pan; // смещение в image-space (целое изображение двигается)

	// Drag state
	bool _isDragging{ false };
	QPointF _lastMousePos;
	QPointF _lastPan;
};
