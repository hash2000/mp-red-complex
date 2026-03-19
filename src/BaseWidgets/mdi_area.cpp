#include "BaseWidgets/mdi_area.h"
#include <QMdiSubWindow>
#include <QWidget>
#include <QScrollBar>
#include <QScrollArea>
#include <QWheelEvent>
#include <QAbstractScrollArea>
#include <QCoreApplication>

MdiArea::MdiArea(QWidget* parent)
	: QMdiArea(parent) {
}

void MdiArea::wheelEvent(QWheelEvent* event) {
	// Получаем активное подокно
	QMdiSubWindow* subWindow = this->activeSubWindow();
	if (!subWindow) {
		// Если нет активного подокна, передаём событие дальше
		QMdiArea::wheelEvent(event);
		return;
	}

	// Получаем виджет подокна
	QWidget* widget = subWindow->widget();
	if (!widget) {
		QMdiArea::wheelEvent(event);
		return;
	}

	// Проверяем, есть ли у виджета или его детей вертикальный скролл
	bool hasVerticalScroll = false;

	// Проверяем сам виджет (если это QAbstractScrollArea или наследник)
	if (auto scrollArea = qobject_cast<QAbstractScrollArea*>(widget)) {
		if (QScrollBar* scrollBar = scrollArea->verticalScrollBar()) {
			hasVerticalScroll = true;
		}
	}

	// Если у виджета нет вертикального скролла, проверяем дочерние QScrollArea
	if (!hasVerticalScroll) {
		QList<QScrollArea*> children = widget->findChildren<QScrollArea*>();
		for (QScrollArea* scrollArea : children) {
			if (QScrollBar* scrollBar = scrollArea->verticalScrollBar()) {
				hasVerticalScroll = true;
				break;
			}
		}
	}

	// Если есть вертикальный скролл — всегда передаём событие подокну
	// и НЕ даём всплывать дальше (даже если скролл достиг границы)
	if (hasVerticalScroll) {
		// Передаём событие подокну
		QCoreApplication::sendEvent(widget, event);
		event->accept();
		return;
	}

	// Если нет вертикального скролла — позволяем событию всплывать
	QMdiArea::wheelEvent(event);
}
