#include "ResourcesTool/widgets/hex/hex_dump_widget.h"
#include "ResourcesTool/widgets/hex/hex_dump_view.h"
#include "ResourcesTool/widgets/hex/hex_control_panel.h"
#include <QHBoxLayout>
#include <QSplitter>

HexDumpWidget::HexDumpWidget(std::shared_ptr<DataStream> stream, QWidget *parent)
: BaseTabWidget(parent) {
	auto byteBlock = stream->readBlockAsQByteArray();
	auto view = new HexDumpView;
	view->setData(byteBlock);

	auto control = new HexControlPanel(view, stream);
	auto splitter = new QSplitter(Qt::Horizontal);
	splitter->addWidget(control);
	splitter->addWidget(view);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	auto layout = new QHBoxLayout(this);
	layout->addWidget(splitter);
	setLayout(layout);
}
