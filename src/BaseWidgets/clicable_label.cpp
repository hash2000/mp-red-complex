#include "BaseWidgets/clicable_label.h"
#include <QMouseEvent>

void ClickableLabel::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		emit clicked();
		event->accept();
	}
	else {
		QLabel::mousePressEvent(event);
	}
}

void ClickableLabel::enterEvent(QEnterEvent* event) {
	setCursor(Qt::OpenHandCursor);
	QLabel::enterEvent(event);
}
void ClickableLabel::leaveEvent(QEvent* event) {
	setCursor(Qt::ArrowCursor);
	QLabel::leaveEvent(event);
}
