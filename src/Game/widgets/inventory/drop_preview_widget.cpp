#include "Game/widgets/inventory/drop_preview_widget.h"
#include <QPainter>

class DropPreviewWidget::Private {
public:
	Private(DropPreviewWidget* parent)
		: q(parent) {
	}

	DropPreviewWidget* q;
	QRect previewRect;
	bool canPlace = false;
	bool visible = false;
};

DropPreviewWidget::DropPreviewWidget(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this))
{
	setAttribute(Qt::WA_TransparentForMouseEvents, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	hide();
}

DropPreviewWidget::~DropPreviewWidget() = default;

void DropPreviewWidget::showPreview(const QRect& rect, bool canPlace) {
	d->previewRect = rect;
	d->canPlace = canPlace;
	d->visible = true;
	show();
	raise();
	update();
}

void DropPreviewWidget::hidePreview() {
	d->visible = false;
	hide();
}

void DropPreviewWidget::paintEvent(QPaintEvent* event) {
	if (!d->visible) {
		return;
	}

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);

	// Зелёный = можно разместить, Красный = нельзя
	QColor fillColor = d->canPlace
		? QColor(34, 197, 94, 80)   // green-400
		: QColor(239, 68, 68, 80);  // red-500

	QColor borderColor = d->canPlace
		? QColor(34, 197, 94, 220)
		: QColor(239, 68, 68, 220);

	painter.fillRect(d->previewRect, fillColor);
	painter.setPen(QPen(borderColor, 2));
	painter.drawRect(d->previewRect.adjusted(0, 0, -1, -1));

	// Внутренняя сетка для визуализации размера
	painter.setPen(QColor(255, 255, 255, 100));
	for (int x = d->previewRect.left() + 32; x < d->previewRect.right(); x += 32) {
		painter.drawLine(x, d->previewRect.top(), x, d->previewRect.bottom());
	}
	for (int y = d->previewRect.top() + 32; y < d->previewRect.bottom(); y += 32) {
		painter.drawLine(d->previewRect.left(), y, d->previewRect.right(), y);
	}
}
