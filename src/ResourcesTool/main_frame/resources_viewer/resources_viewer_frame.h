#pragma once
#include "ResourcesTool/main_frame/resources_viewer/stream_widget_selector.h"
#include "ResourcesTool/main_frame/resources_viewer/tabs_controller.h"
#include "Engine/main_frame.h"
#include "Resources/resources.h"
#include <QMainWindow>
#include <QTreeView>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QLabel>
#include <QMenu>
#include <QListWidget>
#include <QTabWidget>
#include <memory>

class ResourcesViewerFrame : public MainFrame {
public:
	ResourcesViewerFrame(std::shared_ptr<Resources> resources);

	virtual ~ResourcesViewerFrame() = default;

private:
	void setupTabs();
	void setupHomaPageTab();
	void setupAssetsTree();
	void setupSelector();
	void setupView();
	void populateAssetsTree();
	void configureAssetsTree();
	void attachMenuAction(QMenu &menu, const QString &name, void (ResourcesViewerFrame::*slot)());
	void createTab(QWidget* widget, const QString& type, const QString& id, const QString& title);

private slots:
	void onItemDoubleClicked(const QModelIndex &index);
	void onCustomContextMenuRequested(const QPoint &pos);
	void onItemMenuHexView();
	void onItemMenuTextView();
	void onContainerExtract();
	void onBeforeStreamSelection(const QVariantMap& params);
	void onAssetsTreeExpand(const QModelIndex& index);
	void onRequestTabCreation(const QVariantMap& params);

private:
	std::shared_ptr<Resources> _resources;
	std::shared_ptr<StreamWidgetSelector> _selector;
	std::shared_ptr<TabsController> _tabs;
	QTreeView *_assetsView;
	QStandardItemModel *_assetsModel;
	QListWidget *_actionsList;
	QTabWidget *_centerTabs;
};

