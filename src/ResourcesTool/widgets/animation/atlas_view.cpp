#include "ResourcesTool/widgets/animation/atlas_view.h"
#include "DataFormat/data_format/frm/atlas_builder.h"
#include <QPainter>
#include <QWheelEvent>

AtlasView::AtlasView(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("AtlasView");
	setAttribute(Qt::WA_OpaquePaintEvent, false);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setMouseTracking(true);
}

void AtlasView::setup(
	std::shared_ptr<const Proto::Animation> animation,
	std::shared_ptr<const Proto::Pallete> pallete)
{
	_animation = animation;
	_pallete = pallete;
	_direction = 0;
	setupFrames();
}

void AtlasView::setDirection(size_t direction) {
	if (direction >= directionsCount()) {
		return;
	}

	_direction = direction;
	setupFrames();
}

void AtlasView::showRuler(bool show) {
	_showRuller = show;
	update();
}

const Proto::Animation* AtlasView::animation() const noexcept {
	return _animation.get();
}

const Proto::Direction& AtlasView::direction(size_t idx) const {
	return _animation->directions[idx];
}

const Proto::Frame& AtlasView::frame(size_t dirIdx, size_t frameIdx) const {
	return _animation->directions[dirIdx].frames[frameIdx];
}

void AtlasView::setOffset(int x, int y) {
	// В .frm: offsetX/Y — это смещение *изображения* относительно базовой точки (0,0)
	// То есть: изображение находится в прямоугольнике (offsetX, offsetY, width, height)
	// → Мы сохраняем это в _imageRect
	if (!_image.isNull()) {
		_imageRect = QRectF(x, y, _image.width(), _image.height());
		update();
	}
}

void AtlasView::resetView() {
	_scale = 1.0;
	_pan = QPointF(0, 0);
	update();
}

void AtlasView::fitToView() {
	if (_image.isNull() || _imageRect.isEmpty()) {
		return;
	}

	QRectF widgetRect = rect();
	QRectF imageSceneRect = _imageRect;

	// Масштабируем так, чтобы изображение поместилось
	qreal scaleX = widgetRect.width() / imageSceneRect.width();
	qreal scaleY = widgetRect.height() / imageSceneRect.height();
	qreal newScale = qMin(scaleX, scaleY) * 0.9; // 10% padding

	// Центрируем
	_scale = qBound(0.1, newScale, 10.0);
	_pan = imageSceneRect.center() - widgetRect.center() / _scale;

	update();
}

size_t AtlasView::directionsCount() const {
	if (!_animation) {
		return 0;
	}

	return _animation->directions.size();
}

size_t AtlasView::animationsCount(int direction) const {
	if (!_animation) {
		return 0;
	}

	if (direction >= _animation->directions.size()) {
		return 0;
	}

	return _animation->directions[direction].frames.size();
}

void AtlasView::setupFrames() {

	const auto& frames = _animation->directions[_direction].frames;

	DataFormat::Frm::AtlasBuilder atlas;
	atlas.build(frames, *_pallete);
	_image = atlas.atlas();
	updateImageRect();
	update();
}

void AtlasView::updateImageRect() {
	if (_image.isNull()) {
		_imageRect = QRectF();
	}
	else {
		_imageRect = QRectF(0, 0, _image.width(), _image.height());
	}
}

QPointF AtlasView::mapFromImageToWidget(const QPointF& imagePos) const {
	return (imagePos - _pan) * _scale;
}

QPointF AtlasView::mapFromWidgetToImage(const QPointF& widgetPos) const {
	return widgetPos / _scale + _pan;
}

void AtlasView::drawBackgroundRulerLines(QPainter& painter)
{
	if (!_showRuller) {
		return;
	}

	const auto oldPen = painter.pen();
	painter.setPen(QPen(Qt::red, 3, Qt::SolidLine));
	painter.drawLine(QLine(0, 0, 100, 0));

	painter.setPen(QPen(Qt::blue, 1, Qt::SolidLine));
	painter.drawLine(QLine(0, 0, 0, 100));

	painter.setPen(oldPen);
}

void AtlasView::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.fillRect(rect(), Qt::darkGray);

	QTransform transform;
	transform.translate(width() / 2.0, height() / 2.0);
	transform.scale(_scale, _scale);
	transform.translate(-_pan.x(), -_pan.y());
	painter.setTransform(transform);

	if (!_image.isNull() && !_imageRect.isEmpty()) {
		painter.drawImage(_imageRect.topLeft(), _image);
	}

	drawBackgroundRulerLines(painter);
}

void AtlasView::resizeEvent(QResizeEvent* event)
{
}

void AtlasView::wheelEvent(QWheelEvent* event)
{
	if (!_imageRect.isValid()) {
		event->ignore();
		return;
	}

	// 1. Позиция курсора в image-space (до zoom)
	QPointF cursorImageBefore = mapFromWidgetToImage(event->position());

	// 2. Меняем масштаб
	qreal delta = event->angleDelta().y();
	if (delta == 0) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
		delta = event->pixelDelta().y();
#endif
	}

	if (delta == 0) {
		event->ignore();
		return;
	}

	const qreal scaleFactor = 1.125;
	qreal newScale = _scale * (delta > 0 ? scaleFactor : 1.0 / scaleFactor);
	newScale = qBound(0.1, newScale, 10.0);
	if (qFuzzyCompare(_scale, newScale)) return;

	// 3. Применяем zoom к курсору
	_scale = newScale;
	QPointF cursorImageAfter = mapFromWidgetToImage(event->position());

	// 4. Корректируем pan, чтобы курсор остался на той же точке изображения
	_pan += (cursorImageAfter - cursorImageBefore);

	event->accept();
	update();
}

void AtlasView::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		_isDragging = true;
		_lastMousePos = event->position();
		_lastPan = _pan;
		setCursor(Qt::ClosedHandCursor);
	}
	event->accept();
}

void AtlasView::mouseMoveEvent(QMouseEvent* event) {
	if (_isDragging) {
		QPointF delta = (event->position() - _lastMousePos) / _scale;
		_pan = _lastPan - delta; // drag: image moves opposite to mouse
		update();
	}
	event->accept();
}

void AtlasView::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		_isDragging = false;
		setCursor(Qt::ArrowCursor);
	}
	event->accept();
}
