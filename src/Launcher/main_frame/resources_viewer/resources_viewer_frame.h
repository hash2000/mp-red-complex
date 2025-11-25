#pragma once
#include "Launcher/main_frame.h"
#include "Launcher/main_frame/resources_viewer/stream_widget_selector.h"
#include "Resources/resources.h"
#include <QMainWindow>
#include <QTreeView>
#include <QStandardItemModel>
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
	void setupAssetsTree();
	void setupCentralWidget();
	void setupView();
	void populateAssetsTree();
	void configureAssetsTree();

private slots:
	void onItemDoubleClicked(const QModelIndex &index);
	void onCustomContextMenuRequested(const QPoint &pos);
	void onItemMenuHexView();
	void onItemMenuTextView();
	void onBeforeStreamSelection(const QString& suffix, std::optional<std::shared_ptr<DataStream>> stream);

private:
	std::shared_ptr<Resources> _resources;
	std::unique_ptr<StreamWidgetSelector> _selector;
	QTreeView *_assetsView;
	QStandardItemModel *_assetsModel;
	QListWidget *_actionsList;
  QVBoxLayout *_actionsLayout;
	QVBoxLayout *_centerLayout;
  QWidget *_actionsPanel;
	QWidget *_centerPanel;
};

