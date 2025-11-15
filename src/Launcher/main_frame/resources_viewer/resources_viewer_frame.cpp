#include "Launcher/main_frame/resources_viewer/resources_viewer_frame.h"
#include "Launcher/resources/model/assets_model_builder.h"
#include "Launcher/resources/model/assets_model.h"
#include <QSplitter>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <qnamespace.h>

ResourcesViewerFrame::ResourcesViewerFrame(std::shared_ptr<Resources> &resources)
	: _resources(resources) {
	setupAssetsTree();
	setupCentralWidget();
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
	_centerStack = new QStackedWidget;
  _hexView = new HexDumpWidget;
  _imageLabel = new QLabel;
  _imageLabel->setAlignment(Qt::AlignCenter);
  _emptyWidget = new QWidget;

	_centerStack->addWidget(_emptyWidget);
  _centerStack->addWidget(_hexView);
  _centerStack->addWidget(_imageLabel);
}

void ResourcesViewerFrame::setupAssetsTree() {
	_assetsModel = new QStandardItemModel(this);
  _assetsModel->setHorizontalHeaderLabels({"Assets"});

	_assetsView = new QTreeView;
  _assetsView->setModel(_assetsModel);
  _assetsView->setContextMenuPolicy(Qt::CustomContextMenu);
  _assetsView->setHeaderHidden(true);

	connect(_assetsView, &QTreeView::doubleClicked, this, &ResourcesViewerFrame::onItemDoubleClicked);
}

void ResourcesViewerFrame::setupActionPanel() {
	_actionsPanel = new QWidget;
  _actionsLayout = new QVBoxLayout;
  _actionsPanel->setLayout(_actionsLayout);

  _actionsLayout->addWidget(new QLabel("<b>Actions:</b>"));
  _actionsLayout->addWidget(new QPushButton("Edit"));
  _actionsLayout->addWidget(new QPushButton("Delete"));
  _actionsLayout->addWidget(new QPushButton("Export"));
  _actionsLayout->addStretch();
}

void ResourcesViewerFrame::onItemDoubleClicked(const QModelIndex &index) {
	const auto item = _assetsModel->itemFromIndex(index);
	if (item) {
		const auto container = index.data(static_cast<int>(AssetsViewItemRole::ContainerName)).toString();
		const auto path = index.data(static_cast<int>(AssetsViewItemRole::FullPath)).toString();
		const auto suffix = index.data(static_cast<int>(AssetsViewItemRole::Suffix)).toString();
		const auto type = index.data(static_cast<int>(AssetsViewItemRole::Type)).value<AssetsViewItemType>();

		if (type == AssetsViewItemType::File) {
			displayModel(item, container, path, suffix);
		}
	}
}

void ResourcesViewerFrame::displayModel(const QStandardItem *item,
	const QString &container,
	const QString &path,
	const QString &suffix) {
	const auto stream = _resources->getStream(container, path);
	if (!stream) {
		return;
	}

	auto widget = buildWidget(suffix, stream.value());
	if (!widget) {
		return;
	}

	_centerStack->setCurrentWidget(widget);
}

QWidget* ResourcesViewerFrame::buildWidget(const QString &suffix, DataStream &stream) const {
	auto block = stream.readBlockAsQByteArray();
	_hexView->setByteArray(block);
	return _hexView;
}
