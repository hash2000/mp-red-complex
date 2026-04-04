#include "Game/widgets/warmup/warmup_widget.h"
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QVBoxLayout>
#include <memory>

class WarmupWidget::Private {
public:
	Private(WarmupWidget* parent) : q(parent) {}
	WarmupWidget* q;
	QOpenGLWidget* glWidget = nullptr;
};

WarmupWidget::WarmupWidget(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	
	// Создаём QOpenGLWidget - это инициализирует OpenGL контекст
	d->glWidget = new QOpenGLWidget(this);
	
	auto* layout = new QVBoxLayout(this);
	layout->addWidget(d->glWidget);
	layout->setContentsMargins(0, 0, 0, 0);
	
	setMinimumSize(100, 100);
}

WarmupWidget::~WarmupWidget() = default;
