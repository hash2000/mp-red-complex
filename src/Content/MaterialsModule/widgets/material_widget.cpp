#include "Content/MaterialsModule/widgets/material_widget.h"
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
	connect(this, &BaseOpenGLWidget::beginFrame, this, &MaterialWidget::onBerginFrame);
	connect(this, &BaseOpenGLWidget::initializeContext, this, &MaterialWidget::onInitializeContext);
}

MaterialWidget::~MaterialWidget() = default;

void MaterialWidget::onInitializeContext() {
	d->initializeOpenGLFunctions();


}

void MaterialWidget::onBerginFrame() {

}
