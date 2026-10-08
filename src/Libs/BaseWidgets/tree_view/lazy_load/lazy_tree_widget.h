#pragma once

#include <QWidget>

using LazyTreeNodePtr = std::shared_ptr<class ILazyTreeNode>;
using LazyTreeNodeList = std::vector<LazyTreeNodePtr>;
class QStandardItem;
class ILazyNodesDataProvider;

class LazyTreeWidget : public QWidget {
	Q_OBJECT
public:
	explicit LazyTreeWidget(ILazyNodesDataProvider* provider, QWidget* parent = nullptr);
	~LazyTreeWidget() override;

	bool updateNode(const QVariant& nodeId, const LazyTreeNodePtr& nodeData);
	void refreshAll(); // Полное обновление дерева

private slots:
	void onSearchTextChanged(const QString& text);
	void onSearchPerform();
	void onAddNode();
	void onDeleteNode();
	void onEditNode();
	void onShowSearch();
	void onBackToNormalView();
	void onRefreshNode();
	void onContextMenu(const QPoint& pos);

signals:
	void nodeMoved(const LazyTreeNodePtr& node, const LazyTreeNodePtr& newParent);
	void nodeRenamed(const LazyTreeNodePtr& node, const QString& newName);
	void nodeRemoved(const LazyTreeNodePtr& node);
	void nodeAdded(const LazyTreeNodePtr& node);
	void emptyAreaClicked();
	void nodeActivated(const LazyTreeNodePtr& node);
	void selectionChanged(const LazyTreeNodePtr& node);

private:
	class Private;
	std::unique_ptr<Private> d;
};
