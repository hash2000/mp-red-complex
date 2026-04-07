#include "Game/widgets/textures/zoomable_image_view.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>

class ZoomableImageView::Private {
public:
	Private(ZoomableImageView* parent) : q(parent) {}
	ZoomableImageView* q;

	QPixmap pixmap;
	qreal zoomLevel = 1.0;
	qreal minZoom = 0.1;
	qreal maxZoom = 10.0;
	QPointF panOffset;
	QPoint lastMousePos;
	bool isPanning = false;

	void adjustOffsets();
};

void ZoomableImageView::Private::adjustOffsets() {
	if (pixmap.isNull()) {
		panOffset = QPointF(0, 0);
		return;
	}

	const QSizeF scaledSize = QSizeF(pixmap.size()) * zoomLevel;
	panOffset.setX((q->width() - scaledSize.width()) / 2);
	panOffset.setY((q->height() - scaledSize.height()) / 2);
}

ZoomableImageView::ZoomableImageView(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	setMouseTracking(true);
	setMinimumSize(200, 200);
}

ZoomableImageView::~ZoomableImageView() = default;

void ZoomableImageView::setPixmap(const QPixmap& pixmap) {
	d->pixmap = pixmap;
	resetZoom();
	update();
}

void ZoomableImageView::resetZoom() {
	if (d->pixmap.isNull()) {
		d->zoomLevel = 1.0;
		d->panOffset = QPointF(0, 0);
		return;
	}

	// Масштабируем так, чтобы изображение вписывалось в виджет
	const qreal scaleX = static_cast<qreal>(width()) / d->pixmap.width();
	const qreal scaleY = static_cast<qreal>(height()) / d->pixmap.height();
	d->zoomLevel = qMin(scaleX, scaleY);

	// Центрируем
	d->adjustOffsets();
}

void ZoomableImageView::paintEvent(QPaintEvent* /*event*/) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	if (d->pixmap.isNull()) {
		painter.setPen(Qt::white);
		painter.drawText(rect(), Qt::AlignCenter, "Выберите текстуру");
		return;
	}

	const QSizeF scaledSize = QSizeF(d->pixmap.size()) * d->zoomLevel;
	const QPoint topLeft(d->panOffset.toPoint());

	painter.drawPixmap(QRect(topLeft, scaledSize.toSize()), d->pixmap);

	// Рисуем рамку
	painter.setPen(QPen(QColor("#4a5568"), 1));
	painter.drawRect(QRect(topLeft, scaledSize.toSize()));
}

void ZoomableImageView::wheelEvent(QWheelEvent* event) {
	if (d->pixmap.isNull()) {
		return;
	}

	const QPointF delta = event->angleDelta();
	const qreal factor = delta.y() > 0 ? 1.15 : 1.0 / 1.15;

	const qreal newZoom = d->zoomLevel * factor;
	if (newZoom < d->minZoom || newZoom > d->maxZoom) {
		return;
	}

	// Масштабируем относительно позиции курсора
	const QPointF mousePos = event->position();
	const QPointF imagePos = (mousePos - d->panOffset) / d->zoomLevel;

	d->zoomLevel = newZoom;
	d->panOffset = mousePos - imagePos * d->zoomLevel;

	update();
	emit zoomChanged(d->zoomLevel);
}

void ZoomableImageView::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton && !d->pixmap.isNull()) {
		d->isPanning = true;
		d->lastMousePos = event->pos();
		setCursor(Qt::ClosedHandCursor);
	}
}

void ZoomableImageView::mouseMoveEvent(QMouseEvent* event) {
	if (d->isPanning) {
		const QPoint delta = event->pos() - d->lastMousePos;
		d->panOffset += delta;
		d->lastMousePos = event->pos();
		update();
	}
}

void ZoomableImageView::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		d->isPanning = false;
		setCursor(Qt::ArrowCursor);
	}
}
