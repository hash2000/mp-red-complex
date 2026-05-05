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

	// Настройки сетки
	bool gridEnabled = true;  // По умолчанию сетка включена
	int gridSizeX = 16;
	int gridSizeY = 16;
	int selectedTileId = -1;
	QList<int> selectedTileIds;  // Множественное выделение

	void adjustOffsets();
	int tileAtPosition(const QPointF& pos) const;
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

int ZoomableImageView::Private::tileAtPosition(const QPointF& pos) const {
	if (pixmap.isNull()) {
		return -1;
	}

	// Переводим позицию из экранных координат в координаты изображения
	const QPointF imagePos = (pos - panOffset) / zoomLevel;

	if (imagePos.x() < 0 || imagePos.y() < 0 ||
	    imagePos.x() > pixmap.width() || imagePos.y() > pixmap.height()) {
		return -1;
	}

	const qreal tileWidth = static_cast<qreal>(pixmap.width()) / gridSizeX;
	const qreal tileHeight = static_cast<qreal>(pixmap.height()) / gridSizeY;

	const int tileX = static_cast<int>(imagePos.x() / tileWidth);
	const int tileY = static_cast<int>(imagePos.y() / tileHeight);

	if (tileX < 0 || tileX >= gridSizeX || tileY < 0 || tileY >= gridSizeY) {
		return -1;
	}

	return tileY * gridSizeX + tileX;
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

void ZoomableImageView::setGridEnabled(bool enabled) {
	d->gridEnabled = enabled;
	update();
}

void ZoomableImageView::setGridSize(int gridSizeX, int gridSizeY) {
	d->gridSizeX = gridSizeX;
	d->gridSizeY = gridSizeY;
	update();
}

void ZoomableImageView::setSelectedTileId(int tileId) {
	d->selectedTileId = tileId;
	update();
}

void ZoomableImageView::setSelectedTileIds(const QList<int>& tileIds) {
	d->selectedTileIds = tileIds;
	update();
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

	const qreal tileWidth = scaledSize.width() / d->gridSizeX;
	const qreal tileHeight = scaledSize.height() / d->gridSizeY;

	// Рисуем сетку
	if (d->gridEnabled) {
		painter.setRenderHint(QPainter::Antialiasing, false);
		// Вертикальные линии
		for (int x = 0; x <= d->gridSizeX; ++x) {
			const qreal xPos = topLeft.x() + x * tileWidth;
			painter.drawLine(QLineF(xPos, topLeft.y(), xPos, topLeft.y() + scaledSize.height()));
		}

		// Горизонтальные линии
		for (int y = 0; y <= d->gridSizeY; ++y) {
			const qreal yPos = topLeft.y() + y * tileHeight;
			painter.drawLine(QLineF(topLeft.x(), yPos, topLeft.x() + scaledSize.width(), yPos));
		}
	}

	// Подсветка выбранного тайла (одиночное выделение)
	if (d->selectedTileId >= 0) {
		const int tileX = d->selectedTileId % d->gridSizeX;
		const int tileY = d->selectedTileId / d->gridSizeX;

		const QRectF tileRect(
			topLeft.x() + tileX * tileWidth,
			topLeft.y() + tileY * tileHeight,
			tileWidth,
			tileHeight
		);

		// Полупрозрачная подсветка
		painter.setPen(QPen(QColor("#4299e1"), 2));
		painter.setBrush(QColor(66, 153, 225, 60));
		painter.drawRect(tileRect);
	}

	// Подсветка множественного выделения
	for (int tileId : d->selectedTileIds) {
		if (tileId < 0) continue;

		const int tileX = tileId % d->gridSizeX;
		const int tileY = tileId / d->gridSizeX;

		const QRectF tileRect(
			topLeft.x() + tileX * tileWidth,
			topLeft.y() + tileY * tileHeight,
			tileWidth,
			tileHeight
		);

		// Полупрозрачная подсветка (зелёный для множественного)
		painter.setPen(QPen(QColor("#48bb78"), 2));
		painter.setBrush(QColor(72, 187, 120, 60));
		painter.drawRect(tileRect);
	}
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
		const int tileId = d->tileAtPosition(event->position());
		if (tileId >= 0) {
			const bool ctrlPressed = (event->modifiers() & Qt::ControlModifier);
			// НЕ сбрасываем selectedTileId здесь - это делает внешний виджет
			update();
			emit tileClicked(tileId, ctrlPressed);
			return;
		}
	}

	if (event->button() == Qt::RightButton && !d->pixmap.isNull()) {
		d->isPanning = true;
		d->lastMousePos = event->pos();
		setCursor(Qt::ClosedHandCursor);
		return;
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
	if (event->button() == Qt::RightButton) {
		d->isPanning = false;
		setCursor(Qt::ArrowCursor);
	}
}
