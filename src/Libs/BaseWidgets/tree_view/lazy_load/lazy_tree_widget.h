#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <QWidget>

class QStandardItem;
class ILazyNodesDataProvider;
class ILazyTreeNodeActionHandler;

class LazyTreeWidget : public QWidget {
	Q_OBJECT
public:
	explicit LazyTreeWidget(ILazyNodesDataProvider* provider, QWidget* parent = nullptr);
	~LazyTreeWidget() override;

	void setActionHandler(ILazyTreeNodeActionHandler* actionHandler);
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
	void onItemChanged(QStandardItem* item);

signals:
	void nodeEditRequested(const LazyTreeNodePtr& node);
	void nodeActivated(const QVariant& nodeId);
	void nodeSelectionChanged(const LazyTreeNodePtr& node);

private:
	class Private;
	std::unique_ptr<Private> d;
};
