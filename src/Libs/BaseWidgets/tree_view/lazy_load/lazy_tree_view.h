#pragma once
#include <QAbstractItemDelegate>
#include <QPersistentModelIndex>
#include <QTreeView>

#include <memory>
#include <vector>

using LazyTreeNodePtr = std::shared_ptr<class ILazyTreeNode>;
using LazyTreeNodeList = std::vector<LazyTreeNodePtr>;

class ILazyNodesDataProvider;
class QStandardItem;

/// Дерево с ленивой загрузкой узлов.
///
/// Требования к данным:
///  - children() == std::nullopt — лист;
///  - children() == пустой список — узел имеет детей, они загружаются
///    у поставщика при раскрытии;
///  - children() — непустой список — дети переданы заранее, при раскрытии
///    дополнительный запрос не выполняется;
///  - expanded() задаёт начальное состояние узла.
///
/// Добавление узла создаёт временный элемент без имени. Имя проверяется и
/// сохраняется при завершении редактирования; пустое имя удаляет элемент.
class LazyTreeView : public QTreeView {
	Q_OBJECT
public:
	explicit LazyTreeView(ILazyNodesDataProvider* provider, QWidget* parent = nullptr);
	~LazyTreeView() override;

	ILazyNodesDataProvider* provider() const;

	/// Полная перезагрузка дерева.
	void setNodes(const LazyTreeNodeList& nodes);

	/// Добавить детей в существующий узел (пустой parentId — корень).
	void appendChildren(const QVariant& parentId, const LazyTreeNodeList& newNodes);

	/// Заменить детей существующего узла (пустой список очищает узел).
	void setChildren(const QVariant& parentId, const LazyTreeNodeList& newNodes);

	/// Поиск элемента по идентификатору узла (nullptr, если не найден).
	QStandardItem* findNode(const QVariant& id) const;

	QStandardItem* itemFromIndex(const QModelIndex& index) const;

	bool isDummyItem(QStandardItem* item) const;
	bool isTemporaryItem(QStandardItem* item) const;

	/// Развернуть узел и всех его родителей, прокрутить к нему.
	void expandAndSelectNode(const QVariant& id);

	/// Добавить узел и запустить редактирование имени.
	bool addNodeAndStartEdit(QStandardItem* parentItem, LazyTreeNodePtr newNode);

	void startEdit(const QModelIndex& index);
	void startEditSelectedNode();

	/// Удалить выбранный узел (сначала из поставщика данных).
	void removeSelectedNode();

	/// Переместить узел к новому родителю. row < 0 — в конец списка детей.
	bool moveNode(const QVariant& nodeId, const QVariant& newParentId, int row = -1);

	LazyTreeNodePtr selectedNode() const;
	QVariant selectedNodeId() const;

	/// Завершить редактирование элемента: сохранить/откатить изменения.
	void finalizeEditItem(QStandardItem* item);

signals:
	void nodeActivated(const LazyTreeNodePtr& node);
	void currentNodeChanged(const LazyTreeNodePtr& node);
	void emptyAreaClicked();
	void nodeAdded(const LazyTreeNodePtr& node);
	void nodeRemoved(const LazyTreeNodePtr& node);
	void nodeRenamed(const LazyTreeNodePtr& node, const QString& newName);
	void nodeMoved(const LazyTreeNodePtr& node, const LazyTreeNodePtr& newParent);
	void errorOccurred(const QString& message);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void startDrag(Qt::DropActions supportedActions) override;
	void dropEvent(QDropEvent* event) override;
	void closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint) override;

private slots:
	void onExpanded(const QModelIndex& index);
	void onNodeActivated(const QModelIndex& index);
	void onSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

private:
	class Private;
	std::unique_ptr<Private> d;
};
