#pragma once
#include "Launcher/main_frame.h"
#include "Launcher/widgets/hex_dump_widget.h"
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

private:
	void displayModel(const QStandardItem *item,
		const QString &container,
		const QString &path,
		const QString &suffix);

	QWidget* buildWidget(const QString &suffix, DataStream &stream) const;

private:
	std::shared_ptr<Resources> _resources;

	QTreeView *_assetsView;
	QStandardItemModel *_assetsModel;

	QStackedWidget *_centerStack;
  HexDumpWidget *_hexView;
  QLabel *_imageLabel;
  QWidget *_emptyWidget;

  QStandardItem *_currentItem;

	QListWidget *_actionsList;
  QVBoxLayout *_actionsLayout;
  QWidget *_actionsPanel;
};

