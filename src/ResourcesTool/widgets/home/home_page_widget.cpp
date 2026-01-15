#include "ResourcesTool/widgets/home/home_page_widget.h"
#include <QPushButton>

HomePageWidget::HomePageWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
	MainFrame* mainFrame, QWidget* parent)
: BaseTabWidget(resources, params, parent)
, _mainFrame(mainFrame) {
	_flowArea = new FlowLayout;
	_flowArea->setSpacing(12);
	_flowArea->setContentsMargins(10, 10, 10, 10);

	setupButtons();

	//_scrollArea = new QScrollArea(this);
	//_scrollArea->setWidgetResizable(true);

	//_scrollArea->addScrollBarWidget(_flowArea, Qt::AlignmentFlag::AlignLeft);
	//auto* layout = new QVBoxLayout(this);
	//layout->addWidget(_scrollArea);
	setLayout(_flowArea);
}

void HomePageWidget::setupButtons() {
	addButton(QVariantMap{
		{"description", "Maps editor"},
		{"type", "map_editor"},
		});

	addButton(QVariantMap{
		{"description", "Particles editor"},
		{"type", "particles_editor"},
		});

	addButton(QVariantMap{
		{"description", "Shaders editor"},
		{"type", "shaders_editor"},
		});
}

void HomePageWidget::addButton(const QVariantMap& params) {
	auto* button = new QPushButton(this);
	button->setIconSize(QSize(64, 64));
//	button->setIcon(QIcon(icons.value(i, ":/icons/default.png")));
	button->setText(params.value("description").toString());
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	button->setMinimumHeight(80);

	connect(button, &QPushButton::clicked, this, [this, params]() {
		emit requestTabCreation(params);
		});

	_flowArea->addWidget(button);
}
