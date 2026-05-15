#include "Game/widgets/materials/material_widget.h"
#include <QOpenGLFunctions_4_5_Core>
#include <memory>

class MaterialWidget::Private : public QOpenGLFunctions_4_5_Core {
public:
	Private(MaterialWidget* parent) : q(parent) {}
	MaterialWidget* q;
};

MaterialWidget::MaterialWidget(QWidget* parent)
	: BaseOpenGLWidget(parent)
	, d(std::make_unique<Private>(this)) {
}

MaterialWidget::~MaterialWidget() = default;

void MaterialWidget::initializeGL() {
	BaseOpenGLWidget::initializeGL();
	// Инициализация OpenGL для материалов
}

void MaterialWidget::paintGL() {
	BaseOpenGLWidget::paintGL();
	// Отрисовка материалов
}
