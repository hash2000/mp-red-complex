#include "ResourcesTool/widgets/animation/atlas_widget.h"
#include "ResourcesTool/widgets/animation/atlas_view.h"
#include "ResourcesTool/widgets/animation/atlas_view_control_panel.h"
#include "DataFormat/data_format/frm/data_reader.h"
#include "DataFormat/data_format/pal/data_reader.h"
#include <QSplitter>

AtlasWidget::AtlasWidget(std::shared_ptr<Resources> resources, const QVariantMap& params, QWidget* parent)
: BaseTabWidget(resources, params, parent) {
	auto view = new AtlasView;

	const auto suffix = params.value("suffix").toString();

	auto frmBlock = currentStream();
	auto animation = std::make_shared<Proto::Animation>();
	DataFormat::Frm::DataReader reader(*frmBlock);
	reader.read(*animation, suffix);

	auto palleteBlockOpt = resources->getStream("master.dat", "color.pal");
	auto palleteBlock = palleteBlockOpt.value()->makeBlockAsStream();
	auto pallete = std::make_shared<Proto::Pallete>();
	DataFormat::Pal::DataReader pallete_reader(*palleteBlock);
	pallete_reader.read(*pallete);

	view->setup(animation, pallete);

	auto control = new AtlasViewControlPanel(view);
	auto splitter = new QSplitter(Qt::Horizontal);
	splitter->addWidget(control);
	splitter->addWidget(view);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	auto layout = new QHBoxLayout(this);
	layout->addWidget(splitter);
	setLayout(layout);
}
