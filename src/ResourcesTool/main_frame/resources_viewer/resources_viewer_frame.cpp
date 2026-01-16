#include "ResourcesTool/main_frame/resources_viewer/resources_viewer_frame.h"
#include "ResourcesTool/main_frame/resources_viewer/menu_actions_builder.h"
#include "ResourcesTool/widgets/home/home_page_widget.h"
#include "Resources/resources/model/assets_model_builder_linear.h"
#include "Resources/resources/model/assets_model_builder_linear_root.h"
#include "ResourcesTool/main_frame/resources_viewer/widget_factory.h"
#include <QSplitter>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <qnamespace.h>
#include <QUuid>

ResourcesViewerFrame::ResourcesViewerFrame(std::shared_ptr<Resources> resources)
	: _resources(resources)
	, _assetsModel(new QStandardItemModel(this))
	, _assetsView(new ResourcesTree)
	, _centerTabs(new QTabWidget) {
	setupSelector();
	setupAssetsTree();
	setupTabs();
	setupHomaPageTab();
	setupView();
	populateAssetsTree();
	configureAssetsTree();
}

void ResourcesViewerFrame::setupTabs() {
	_tabs = std::make_shared<TabsController>(_centerTabs, _resources);
}

void ResourcesViewerFrame::setupHomaPageTab() {
	auto widget = new HomePageWidget(_resources, QVariantMap(), this);
	createTab(widget, "home_page", "home", "Home");

	connect(widget, &HomePageWidget::requestTabCreation,
		this, &ResourcesViewerFrame::onRequestTabCreation);
}

void ResourcesViewerFrame::createTab(QWidget* widget, const QString& type, const QString& id, const QString& title) {
	widget->setProperty("tab.type", type);
	widget->setProperty("tab.id", QString("page://%1").arg(id));
	const auto index = _centerTabs->addTab(widget, title);
	_centerTabs->setCurrentIndex(index);
}

void ResourcesViewerFrame::setupView() {
	QSplitter *splitter = new QSplitter(Qt::Horizontal);
  splitter->addWidget(_centerTabs);
	splitter->addWidget(_assetsView);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 0);

  setCentralWidget(splitter);
  setWindowTitle("Asset Manager");
}

void ResourcesViewerFrame::onRequestTabCreation(const QVariantMap& params) {
	const auto type = params.value("type").toString();
	const auto title = params.value("description").toString();
	const auto id = QUuid::createUuid().toString();

	WidgetsFactory factory(_resources, params);
	auto widget = factory.create(this);
	if (widget == nullptr) {
		return;
	}

	createTab(widget, type, id, title);
}

void ResourcesViewerFrame::populateAssetsTree() {
	_assetsModel->clear();
	_assetsModel->setHorizontalHeaderLabels({"Assets"});

	AssetsModelBuilderLinearRoot(_assetsModel->invisibleRootItem(), _resources)
		.build();
}

void ResourcesViewerFrame::configureAssetsTree() {
	auto root = _assetsModel->invisibleRootItem();
	_assetsView->expand(root->index());
}

void ResourcesViewerFrame::setupSelector() {
	_selector = std::make_shared<StreamWidgetSelector>();

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
	connect(_assetsView, &QTreeView::expanded,
		this, &ResourcesViewerFrame::onAssetsTreeExpand);
}

void ResourcesViewerFrame::onItemDoubleClicked(const QModelIndex &index) {
	const auto item = _assetsModel->itemFromIndex(index);
	_selector->setSelection(item);
	_selector->execute();
}

void ResourcesViewerFrame::onBeforeStreamSelection(const QVariantMap& params) {
	QVariantMap append(params);
	append.insert("action", "add");
	_tabs->execute(append);
}

void ResourcesViewerFrame::onAssetsTreeExpand(const QModelIndex& index) {
	AssetsModelBuilderLinear(_assetsModel->itemFromIndex(index), _resources)
		.build();
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

	MenuActionsBuilder builder(this, _selector);
	auto menu = builder.Build();
	if (!menu->actions().isEmpty()) {
		attachMenuAction(*menu, "action_hex_view", &ResourcesViewerFrame::onItemMenuHexView);
		attachMenuAction(*menu, "action_text_view", &ResourcesViewerFrame::onItemMenuTextView);
		attachMenuAction(*menu, "action_extract_container_to", &ResourcesViewerFrame::onContainerExtract);
		menu->exec(_assetsView->viewport()->mapToGlobal(pos));
	}
}

void ResourcesViewerFrame::attachMenuAction(QMenu &menu, const QString &name, void (ResourcesViewerFrame::*slot)()) {
	for (QAction *action : menu.actions()) {
	  if (action->objectName() == name) {
	    connect(action, &QAction::triggered, this, slot);
	    return;
	  }
	}
}

void ResourcesViewerFrame::onItemMenuHexView() {
	auto selection = _selector->getSelection();
	selection.remove("suffix");
	selection.insert("suffix", "hex");
	selection.insert("action", "add");
	_tabs->execute(selection);
}

void ResourcesViewerFrame::onItemMenuTextView() {
	auto selection = _selector->getSelection();
	selection.remove("suffix");
	selection.insert("suffix", "txt");
	selection.insert("action", "add");
	_tabs->execute(selection);
}

void ResourcesViewerFrame::onContainerExtract() {

}
