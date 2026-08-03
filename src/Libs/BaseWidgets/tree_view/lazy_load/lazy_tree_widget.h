#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"
#include <QWidget>

class LazyTreeViewWidget : public QWidget {
	Q_OBJECT
public:
	explicit LazyTreeViewWidget(ILazyNodesDataProvider* provider, QWidget* parent = nullptr);
	~LazyTreeViewWidget() override;

private slots:
	void onSearchTextChanged(const QString& text);
	void performSearch();
	void onAddNode();
	void onDeleteNode();
	void onShowSearch();
	void onBackToNormalView();
	void showContextMenu(const QPoint& pos);
	void onTreeViewExpanded(const QModelIndex& index);
	void onTreeViewItemActivated(const QModelIndex& index);

signals:
	void activateNode(const LazyTreeNodePtr& node);

private:
	class Private;
	std::unique_ptr<Private> d;
};
