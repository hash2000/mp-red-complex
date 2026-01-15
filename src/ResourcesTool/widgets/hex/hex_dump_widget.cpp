#include "ResourcesTool/widgets/hex/hex_dump_widget.h"
#include "ResourcesTool/widgets/hex/hex_dump_view.h"
#include "ResourcesTool/widgets/hex/hex_control_panel.h"
#include <QHBoxLayout>
#include <QSplitter>

HexDumpWidget::HexDumpWidget(std::shared_ptr<Resources> resources, const QVariantMap& params, QWidget *parent)
: BaseTabWidget(resources, params, parent) {
	const auto containerName = params.value("container.name").toString();
	const auto containerPath = params.value("container.path").toString();

	auto blockOpt = resources->getStream(containerName, containerPath);
	auto block = blockOpt.value()->makeBlockAsStream();

	auto byteBlock = block->readBlockAsQByteArray();
	auto view = new HexDumpView;
	view->setData(byteBlock);

	auto control = new HexControlPanel(view, block);
	auto splitter = new QSplitter(Qt::Horizontal);
	splitter->addWidget(control);
	splitter->addWidget(view);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	auto layout = new QHBoxLayout(this);
	layout->addWidget(splitter);
	setLayout(layout);
}
