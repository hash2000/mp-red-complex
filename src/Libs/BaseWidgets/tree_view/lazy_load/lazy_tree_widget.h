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
	void performSearch();
	void onAddNode();
	void onDeleteNode();
	void onEditNode();
	void onShowSearch();
	void onBackToNormalView();
	void onRefreshNode();
	void showContextMenu(const QPoint& pos);

signals:
	void nodeEditRequested(const LazyTreeNodePtr& node);
	void nodeSelectionChanged(const LazyTreeNodePtr& node);

private:
	class Private;
	std::unique_ptr<Private> d;
};
