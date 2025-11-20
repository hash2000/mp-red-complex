#include "Launcher/main_frame/resources_viewer/resources_viewer_frame.h"
#include "Resources/resources/model/assets_model_builder.h"
#include <QSplitter>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <qnamespace.h>

ResourcesViewerFrame::ResourcesViewerFrame(std::shared_ptr<Resources> &resources)
	: _resources(resources)
	, _centerStack(new QStackedWidget)
	, _assetsModel(new QStandardItemModel(this))
	, _assetsView(new QTreeView)
	, _actionsPanel(new QWidget)
  , _actionsLayout(new QVBoxLayout) {
	setupCentralWidget();
	setupAssetsTree();
	setupActionPanel();
	setupView();
	populateAssetsTree();
	configureAssetsTree();
}

void ResourcesViewerFrame::setupView() {
	QSplitter *splitter = new QSplitter(Qt::Horizontal);
  splitter->addWidget(_assetsView);
  splitter->addWidget(_centerStack);
	splitter->addWidget(_actionsPanel);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 2);
	splitter->setStretchFactor(2, 3);

  setCentralWidget(splitter);
  setWindowTitle("Asset Manager");
}

void ResourcesViewerFrame::populateAssetsTree() {
	_assetsModel->clear();
	_assetsModel->setHorizontalHeaderLabels({"Assets"});

	AssetsModelBuilder(_assetsModel->invisibleRootItem(), _resources)
		.build();
}

void ResourcesViewerFrame::configureAssetsTree() {
	auto root = _assetsModel->invisibleRootItem();
	_assetsView->expand(root->index());
}

void ResourcesViewerFrame::setupCentralWidget() {
	_selector = std::make_unique<StreamWidgetSelector>(_resources, _centerStack, _actionsLayout);
	_selector->buildStackedView();
}

void ResourcesViewerFrame::setupAssetsTree() {
  _assetsModel->setHorizontalHeaderLabels({"Assets"});

  _assetsView->setModel(_assetsModel);
  _assetsView->setContextMenuPolicy(Qt::CustomContextMenu);
  _assetsView->setHeaderHidden(true);
	_assetsView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	connect(_assetsView, &QTreeView::customContextMenuRequested,
		this, &ResourcesViewerFrame::onCustomContextMenuRequested);
	connect(_assetsView, &QTreeView::doubleClicked,
		this, &ResourcesViewerFrame::onItemDoubleClicked);

}

void ResourcesViewerFrame::setupActionPanel() {
  _actionsPanel->setLayout(_actionsLayout);

  _actionsLayout->addWidget(new QLabel("<b>Actions:</b>"));
  _actionsLayout->addWidget(new QPushButton("Edit"));
  _actionsLayout->addWidget(new QPushButton("Delete"));
  _actionsLayout->addWidget(new QPushButton("Export"));
  _actionsLayout->addStretch();
}

void ResourcesViewerFrame::onItemDoubleClicked(const QModelIndex &index) {
	const auto item = _assetsModel->itemFromIndex(index);
	_selector->setSelection(item);
	_selector->displayModel();
}

void ResourcesViewerFrame::onCustomContextMenuRequested(const QPoint &pos) {
  const auto index = _assetsView->indexAt(pos);
  if (!index.isValid()) {
		return;
	}

	const auto item = _assetsModel->itemFromIndex(index);
	if (!item || item == _assetsModel->invisibleRootItem()) {
		return;
	}

	_selector->setSelection(item);
	auto menu = _selector->buildContextMenu(this);
	if (!menu->actions().isEmpty()) {
		for (auto *action : menu->actions()) {
			if (action->objectName() == "action_hex_view") {
				connect(action, &QAction::triggered, this, &ResourcesViewerFrame::onItemMenuHexView);
			}
		}

		menu->exec(_assetsView->viewport()->mapToGlobal(pos));
	}
}

void ResourcesViewerFrame::onItemMenuHexView() {
	_selector->displayHexView();
}

