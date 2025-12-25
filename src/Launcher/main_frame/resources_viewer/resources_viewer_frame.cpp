#include "Launcher/main_frame/resources_viewer/resources_viewer_frame.h"
#include "Launcher/main_frame/resources_viewer/widget_maker.h"
#include "Launcher/main_frame/resources_viewer/menu_actions_builder.h"
#include "Resources/resources/model/assets_model_builder.h"
#include <QSplitter>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <qnamespace.h>

ResourcesViewerFrame::ResourcesViewerFrame(std::shared_ptr<Resources> &resources)
	: _resources(resources)
	, _assetsModel(new QStandardItemModel(this))
	, _assetsView(new QTreeView)
	, _centerTabs(new QTabWidget) {
	setupSelector();
	setupWidgetMaker();
	setupAssetsTree();
	setupView();
	populateAssetsTree();
	configureAssetsTree();
}

void ResourcesViewerFrame::setupWidgetMaker() {
	_widgetMaker = std::make_unique<WidgetMaker>(_selector, _resources, _centerTabs);
}

void ResourcesViewerFrame::setupView() {
	_centerTabs->setTabsClosable(true);

	connect(_centerTabs, &QTabWidget::tabCloseRequested, [&](int index) {
		_centerTabs->removeTab(index);
	});

	QSplitter *splitter = new QSplitter(Qt::Horizontal);
  splitter->addWidget(_centerTabs);
	splitter->addWidget(_assetsView);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 0);

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

void ResourcesViewerFrame::setupSelector() {
	_selector = std::make_shared<StreamWidgetSelector>(_resources);

	connect(_selector.get(), &StreamWidgetSelector::beforeStreamSelection,
		this, &ResourcesViewerFrame::onBeforeStreamSelection);
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

void ResourcesViewerFrame::onItemDoubleClicked(const QModelIndex &index) {
	const auto item = _assetsModel->itemFromIndex(index);
	_selector->setSelection(item);
	_selector->displayModel(*_resources);
}

void ResourcesViewerFrame::onBeforeStreamSelection(const QString& suffix, std::optional<std::shared_ptr<DataStream>> stream) {
	if (!stream) {
		return;
	}

	const auto type = From<WidgetResource>::from(suffix);
	if (!type) {
		return;
	}

	_widgetMaker->make(type.value(), suffix);
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

	MenuActionsBuilder builder(this);

	_selector->setSelection(item);
	auto menu = builder.Build(_selector->getType(), _selector->getSuffix());
	if (!menu->actions().isEmpty()) {
		AttachMenuAction(*menu, "action_hex_view", &ResourcesViewerFrame::onItemMenuHexView);
		AttachMenuAction(*menu, "action_text_view", &ResourcesViewerFrame::onItemMenuTextView);
		AttachMenuAction(*menu, "action_extract_container_to", &ResourcesViewerFrame::onContainerExtract);
		menu->exec(_assetsView->viewport()->mapToGlobal(pos));
	}
}

void ResourcesViewerFrame::AttachMenuAction(QMenu &menu, const QString &name, void (ResourcesViewerFrame::*slot)()) {
	for (QAction *action : menu.actions()) {
	  if (action->objectName() == name) {
	    connect(action, &QAction::triggered, this, slot);
	    return;
	  }
	}
}

void ResourcesViewerFrame::onItemMenuHexView() {
	_widgetMaker->make(WidgetResource::Hex, "");
}

void ResourcesViewerFrame::onItemMenuTextView() {
	_widgetMaker->make(WidgetResource::Text, "");
}

void ResourcesViewerFrame::onContainerExtract() {

}
