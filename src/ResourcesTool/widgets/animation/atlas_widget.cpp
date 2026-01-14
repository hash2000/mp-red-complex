#include "ResourcesTool/widgets/animation/atlas_widget.h"
#include "ResourcesTool/widgets/animation/atlas_view.h"
#include "ResourcesTool/widgets/animation/atlas_view_control_panel.h"
#include <QSplitter>

AtlasWidget::AtlasWidget(
	std::shared_ptr<const Proto::Animation> animation,
	std::shared_ptr<const Proto::Pallete> pallete, QWidget* parent)
: BaseTabWidget(parent) {
	auto view = new AtlasView;
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
