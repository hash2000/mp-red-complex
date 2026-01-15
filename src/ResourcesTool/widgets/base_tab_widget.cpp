#include "ResourcesTool/widgets/base_tab_widget.h"

BaseTabWidget::BaseTabWidget(std::shared_ptr<Resources> resources, const QVariantMap& params, QWidget* parent)
: QWidget(parent)
, _resources(resources)
, _params(params) {
}

std::shared_ptr<DataStream> BaseTabWidget::currentStream() const
{
	const auto containerName = _params.value("container.name").toString();
	const auto containerPath = _params.value("container.path").toString();

	auto blockOpt = _resources->getStream(containerName, containerPath);
	auto block = blockOpt.value()->makeBlockAsStream();
	return block;
}
