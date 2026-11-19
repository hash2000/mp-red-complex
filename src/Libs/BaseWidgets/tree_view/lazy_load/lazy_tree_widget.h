#pragma once
#include <QString>
#include <QVariant>
#include <QWidget>

#include <memory>
#include <vector>

using LazyTreeNodePtr = std::shared_ptr<class ILazyTreeNode>;
using LazyTreeNodeList = std::vector<LazyTreeNodePtr>;

class ILazyNodesDataProvider;
class LazyTreeView;
class QLineEdit;

/// Обёртка над LazyTreeView: панель инструментов и окно поиска.
///
/// Обычный и поисковый вид дерева используют один поставщик данных:
/// поиск выполняет поставщик, поэтому результат содержит нужных предков
/// и раскрывается по тем же правилам, что и обычное дерево.
class LazyTreeWidget : public QWidget {
	Q_OBJECT
public:
	explicit LazyTreeWidget(ILazyNodesDataProvider* provider, QWidget* parent = nullptr);
	~LazyTreeWidget() override;

	ILazyNodesDataProvider* provider() const;
	LazyTreeView* treeView() const;
	LazyTreeView* searchView() const;
	QLineEdit* searchLineEdit() const;

	bool isSearchMode() const;

	/// Подтверждать удаление через диалог (по умолчанию true).
	void setConfirmDelete(bool confirm);
	bool confirmDelete() const;

	/// Сохранить узел у поставщика и обновить его в дереве.
	bool updateNode(const QVariant& nodeId, const LazyTreeNodePtr& nodeData);

	/// Полное обновление дерева.
	void refreshAll();

	/// Сбросить поиск и вернуться к обычному виду.
	void clearSearch();

signals:
	void nodeMoved(const LazyTreeNodePtr& node, const LazyTreeNodePtr& newParent);
	void nodeRenamed(const LazyTreeNodePtr& node, const QString& newName);
	void nodeRemoved(const LazyTreeNodePtr& node);
	void nodeAdded(const LazyTreeNodePtr& node);
	void emptyAreaClicked();
	void nodeActivated(const LazyTreeNodePtr& node);
	void selectionChanged(const LazyTreeNodePtr& node);
	void errorOccurred(const QString& message);

private slots:
	void onSearchTextChanged(const QString& text);
	void onSearchPerform();
	void onAddNode();
	void onDeleteNode();
	void onEditNode();
	void onBackToNormalView();
	void onRefreshNode();
	void onContextMenu(const QPoint& pos);
	void onCurrentNodeChanged(const LazyTreeNodePtr& node);

private:
	class Private;
	std::unique_ptr<Private> d;
};
