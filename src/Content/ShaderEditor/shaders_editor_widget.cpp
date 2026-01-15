#include "Content/ShaderEditor/shaders_editor_widget.h"
#include "Content/ShaderEditor/shader_control_panel.h"
#include "Content/ShaderEditor/shader_view.h"
#include <QSplitter>
#include <QHBoxLayout>

ShadersEditorWidget::ShadersEditorWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
	QWidget* parent)
: QWidget(parent) {
	auto view = new ShaderView(params);
	auto control = new ShaderControlPanel(view);
	auto splitter = new QSplitter(Qt::Horizontal);
	splitter->addWidget(control);
	splitter->addWidget(view);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	auto layout = new QHBoxLayout(this);
	layout->addWidget(splitter);
	setLayout(layout);
}

ShadersEditorWidget::~ShadersEditorWidget() {
}
