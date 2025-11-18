#pragma once
#include "Launcher/main_frame.h"
#include "Launcher/main_frame/resources_viewer/stream_widget_selector.h"
#include "Resources/resources/model/assets_model.h"
#include "Resources/resources.h"
#include <QMainWindow>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStackedWidget>
#include <QTextEdit>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>
#include <QListWidget>
#include <memory>

class ResourcesViewerFrame : public MainFrame {
public:
	ResourcesViewerFrame(std::shared_ptr<Resources> &resources);

	virtual ~ResourcesViewerFrame() = default;

private:
	void setupActionPanel();

	void setupAssetsTree();

	void setupCentralWidget();

	void setupView();

	void populateAssetsTree();

	void configureAssetsTree();

private slots:
	void onItemDoubleClicked(const QModelIndex &index);

	void onCustomContextMenuRequested(const QPoint &pos);

	void onItemMenuHexView();

private:
	std::shared_ptr<Resources> _resources;
	std::unique_ptr<StreamWidgetSelector> _selector;
	QTreeView *_assetsView;
	QStandardItemModel *_assetsModel;
	QStackedWidget *_centerStack;
	QListWidget *_actionsList;
  QVBoxLayout *_actionsLayout;
  QWidget *_actionsPanel;
};

