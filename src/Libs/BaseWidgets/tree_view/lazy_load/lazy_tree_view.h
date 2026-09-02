#pragma once

#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <QTreeView>
#include <QStandardItem>
#include <memory>

class ILazyTreeNodeActionHandler;

class LazyTreeView : public QTreeView {
	Q_OBJECT
public:
	explicit LazyTreeView(QWidget* parent = nullptr);

	~LazyTreeView() override;

	void setActionHandler(ILazyTreeNodeActionHandler* actionHandler);

	// Установка данных (полная перезагрузка)
	void setNodes(const LazyTreeNodeList& nodes);

	// Добавление детей в существующий узел (после загрузки от провайдера)
	void appendChildren(const QVariant& parentId, const LazyTreeNodeList& newNodes);

	// Поиск узла по ID (возвращает nullptr, если не найден)
	QStandardItem* findNode(const QVariant& id) const;

	QStandardItem* itemFromIndex(const QModelIndex& index) const;

	// Развернуть узел и всех его родителей, прокрутить к нему
	void expandAndSelectNode(const QVariant& id);

	// Добавление нового узла и запуск редактирования
	void addNodeAndStartEdit(QStandardItem* parentItem, LazyTreeNodePtr newNode);

	// Удаление выбранного узла
	void removeSelectedNode();

	// Вернуть описатель выбранного узла
	LazyTreeNodePtr selectedNode() const;

	// Вернуть идентификатор выбранного узла
	QVariant selectedNodeId() const;

signals:
	// Запрос данных: виджет-владелец должен загрузить детей и вызвать appendChildren
	void requestFetchChildren(const QVariant& parentId);

	// Пользователь активировал узел (двойной клик / Enter)
	void nodeActivated(const LazyTreeNodePtr& node, const QString& nodeName);

	// Изменился выделенный узел (пустой QVariant, если выделение снято)
	void selectionChanged(const LazyTreeNodePtr& node);

	// Клик по пустому месту
	void emptyAreaClicked();

	void nodeAdded(const LazyTreeNodePtr& node);
	void nodeRemoved(const LazyTreeNodePtr& node);
	void nodeRenamed(const LazyTreeNodePtr& node, const QString& newName);
	void nodeMoved(const LazyTreeNodePtr& node, const QVariant& newParentId);

protected:
	void mousePressEvent(QMouseEvent* event) override;

private slots:
	void onExpanded(const QModelIndex& index);
	void onNodeActivated(const QModelIndex& index);
	void onSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
	void onItemChanged(QStandardItem* item);

private:
	class Private;
	std::unique_ptr<Private> d;
};
