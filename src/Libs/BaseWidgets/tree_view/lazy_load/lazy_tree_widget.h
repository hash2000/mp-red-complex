#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

#include <QWidget>

class LazyTreeViewWidget : public QWidget {
	Q_OBJECT
public:
	explicit LazyTreeViewWidget(ILazyNodesDataProvider* provider, QWidget* parent = nullptr);
	~LazyTreeViewWidget() override;

	bool updateNode(const QVariant& nodeId, const LazyTreeNodePtr& nodeData);
	void refreshAll(); // Полное обновление дерева

private slots:
	void onSearchTextChanged(const QString& text);
	void performSearch();
	void onAddNode();
	void onDeleteNode();
	void onShowSearch();
	void onBackToNormalView();
	void showContextMenu(const QPoint& pos);
	void onTreeViewExpanded(const QModelIndex& index);
	void onNodeActivated(const QModelIndex& index);
	void onSelectionChanged();

signals:
	void nodeEditRequested(const LazyTreeNodePtr& node);
	void nodeActivated(const QVariant& nodeId);
	void nodeSelectionChanged(const QVariant& nodeId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
