#include "Launcher/main_frame/resources_viewer/resources_viewer_frame.h"
#include "resources_viewer_frame.h"
#include <QSplitter>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>

ResourcesViewerFrame::ResourcesViewerFrame() {
	setupAssetsTree();
	setupCentralWidget();
	setupActionPanel();



	QSplitter *splitter = new QSplitter(Qt::Horizontal);
  splitter->addWidget(_assetsView);
  splitter->addWidget(_centerStack);
	splitter->addWidget(_actionsPanel);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 2);
	splitter->setStretchFactor(2, 3);

  setCentralWidget(splitter);
  setWindowTitle("Asset Manager");
	populateAssetsView();
}

void ResourcesViewerFrame::setupCentralWidget() {
	_centerStack = new QStackedWidget;
  _textEditor = new QTextEdit;
  _textEditor->setReadOnly(true);
  _imageLabel = new QLabel;
  _imageLabel->setAlignment(Qt::AlignCenter);
  _emptyWidget = new QWidget;

	_centerStack->addWidget(_emptyWidget);
  _centerStack->addWidget(_textEditor);
  _centerStack->addWidget(_imageLabel);
}

void ResourcesViewerFrame::populateAssetsView() {

}

void ResourcesViewerFrame::setupAssetsTree() {
	_assetsModel = new QStandardItemModel(this);
  _assetsModel->setHorizontalHeaderLabels({"Assets"});

	_assetsView = new QTreeView;
  _assetsView->setModel(_assetsModel);
  _assetsView->setContextMenuPolicy(Qt::CustomContextMenu);
  _assetsView->setHeaderHidden(true);
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
