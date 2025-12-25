#pragma once
#include "Launcher/main_frame.h"
#include "Launcher/main_frame/resources_viewer/stream_widget_selector.h"
#include "Launcher/main_frame/resources_viewer/widget_maker.h"
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
	ResourcesViewerFrame(std::shared_ptr<Resources> &resources);

	virtual ~ResourcesViewerFrame() = default;

private:
	void setupTabs();
	void setupHomaPageTab();
	void setupWidgetMaker();
	void setupAssetsTree();
	void setupSelector();
	void setupView();
	void populateAssetsTree();
	void configureAssetsTree();
	void AttachMenuAction(QMenu &menu, const QString &name, void (ResourcesViewerFrame::*slot)());
	void CloseTabByIndex(int index);

private slots:
	void onItemDoubleClicked(const QModelIndex &index);
	void onCustomContextMenuRequested(const QPoint &pos);
	void onItemMenuHexView();
	void onItemMenuTextView();
	void onContainerExtract();
	void onBeforeStreamSelection(const QString& suffix,
		std::optional<std::shared_ptr<DataStream>> stream);

private:
	std::shared_ptr<Resources> _resources;
	std::shared_ptr<StreamWidgetSelector> _selector;
	std::unique_ptr<WidgetMaker> _widgetMaker;
	QTreeView *_assetsView;
	QStandardItemModel *_assetsModel;
	QListWidget *_actionsList;
	QTabWidget *_centerTabs;
};

