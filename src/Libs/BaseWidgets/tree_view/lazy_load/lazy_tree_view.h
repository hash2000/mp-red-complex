#pragma once
#include <QTreeView>
#include <QStandardItem>
#include <memory>

using LazyTreeNodePtr = std::shared_ptr<class ILazyTreeNode>;
using LazyTreeNodeList = std::vector<LazyTreeNodePtr>;
using LazyTreeNodeStatePtr = std::shared_ptr<class LazyTreeNodeState>;

class ILazyNodesDataProvider;

class LazyTreeView : public QTreeView {
	Q_OBJECT
public:
	explicit LazyTreeView(ILazyNodesDataProvider* provider, QWidget* parent = nullptr);

	~LazyTreeView() override;

	// Установка данных (полная перезагрузка)
	void setNodes(const LazyTreeNodeList& nodes);

	// Добавление детей в существующий узел
	void appendChildren(const QVariant& parentId, const LazyTreeNodeList& newNodes);

	// Перезаписыавет детей в существующий узел
	void setChildren(const QVariant& parentId, const LazyTreeNodeList& newNodes);

	// Поиск узла по ID (возвращает nullptr, если не найден)
	QStandardItem* findNode(const QVariant& id) const;

	// Вернуть запись дерева по индексу
	QStandardItem* itemFromIndex(const QModelIndex& index) const;

	// Развернуть узел и всех его родителей, прокрутить к нему
	void expandAndSelectNode(const QVariant& id);

	// Добавление нового узла и запуск редактирования
	bool addNodeAndStartEdit(QStandardItem* parentItem, LazyTreeNodePtr newNode);

	// Начать редактирование узла
	void startEdit(const QModelIndex& index);

	// Начать редактирование выбранного узла
	void startEditSelectedNode();

	// Удаление выбранного узла
	void removeSelectedNode();

	// Вернуть описатель выбранного узла
	LazyTreeNodePtr selectedNode() const;

	// Вернуть идентификатор выбранного узла
	QVariant selectedNodeId() const;

	// Завершить редактирование узла
	void finalizeEditItem(QStandardItem* item);

signals:
	void nodeActivated(const LazyTreeNodePtr& node);
	void selectionChanged(const LazyTreeNodePtr& node);
	void emptyAreaClicked();
	void nodeAdded(const LazyTreeNodePtr& node);
	void nodeRemoved(const LazyTreeNodePtr& node);
	void nodeRenamed(const LazyTreeNodePtr& node, const QString& newName);
	void nodeMoved(const LazyTreeNodePtr& node, const LazyTreeNodePtr& newParent);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private slots:
	void onExpanded(const QModelIndex& index);
	void onNodeActivated(const QModelIndex& index);
	void onSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
	void onItemChanged(QStandardItem* item);

private:
	class Private;
	std::unique_ptr<Private> d;
};
