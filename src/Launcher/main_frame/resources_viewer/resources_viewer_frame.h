#pragma once
#include "Launcher/main_frame.h"
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
	ResourcesViewerFrame();

	virtual ~ResourcesViewerFrame() = default;

private:
	void setupActionPanel();
	void setupAssetsTree();
	void setupCentralWidget();

	void populateAssetsView();

private:
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

