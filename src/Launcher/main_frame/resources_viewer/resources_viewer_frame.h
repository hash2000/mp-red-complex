#pragma once
#include "Launcher/main_frame.h"
#include "Launcher/resources.h"
#include <QMainWindow>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStackedWidget>
#include <QTextEdit>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>
#include <QListWidget>

class ResourcesViewerFrame : public MainFrame {
public:
	ResourcesViewerFrame(std::shared_ptr<Resources> &resources);

	virtual ~ResourcesViewerFrame() = default;

private:
	void setupActionPanel();

	void setupAssetsTree();

	void setupCentralWidget();

	void setupView();

	void populateAssetsView();

private:
	std::shared_ptr<Resources> _resources;

	QTreeView *_assetsView;
	QStandardItemModel *_assetsModel;

	QStackedWidget *_centerStack;
  QTextEdit *_textEditor;
  QLabel *_imageLabel;
  QWidget *_emptyWidget;

  QStandardItem *_currentItem;

	QListWidget *_actionsList;
  QVBoxLayout *_actionsLayout;
  QWidget *_actionsPanel;
};

