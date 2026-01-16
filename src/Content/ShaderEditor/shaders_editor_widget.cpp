#include "Content/ShaderEditor/shaders_editor_widget.h"
#include "Content/ShaderEditor/shader_control_panel.h"
#include "Content/ShaderEditor/shader_view.h"
#include "DataFormat/data_format/txt/data_reader.h"
#include <QSplitter>
#include <QHBoxLayout>

ShadersEditorWidget::ShadersEditorWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
	QWidget* parent)
: BaseTabWidget(resources, params, parent) {
	auto view = new ShaderView(params);
	_controlPanel = new ShaderControlPanel(view);
	auto splitter = new QSplitter(Qt::Horizontal);
	splitter->addWidget(_controlPanel);
	splitter->addWidget(view);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	connect(this, &BaseTabWidget::requestContainer, this, &ShadersEditorWidget::onRequestContainer);

	auto layout = new QHBoxLayout(this);
	layout->addWidget(splitter);
	setLayout(layout);
}

ShadersEditorWidget::~ShadersEditorWidget() {
}

void ShadersEditorWidget::onRequestContainer(const QVariantMap& params) {
	_params = params;
	const auto containerName = params.value("container.name").toString();
	const auto containerPath = params.value("container.path").toString();
	auto block = currentStream();

	QString source;
	DataFormat::Txt::DataReader reader(*block);
	reader.read(source);

	_controlPanel->applySourceToCurrentTab(containerName, containerPath, source);
}
