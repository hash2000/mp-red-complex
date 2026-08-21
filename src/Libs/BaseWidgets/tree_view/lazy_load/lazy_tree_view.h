#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <QTreeView>
#include <QStandardItem>
#include <memory>


class LazyTreeView : public QTreeView {
	Q_OBJECT
public:
	explicit LazyTreeView(QWidget* parent = nullptr);

	~LazyTreeView() override;

	// Установка данных (полная перезагрузка)
	void setNodes(const LazyTreeNodeList& nodes);

	// Добавление детей в существующий узел (после загрузки от провайдера)
	void appendChildren(const QVariant& parentId, const LazyTreeNodeList& newNodes);

	// Поиск узла по ID (возвращает nullptr, если не найден)
	QStandardItem* findNode(const QVariant& id) const;

	// Развернуть узел и всех его родителей, прокрутить к нему
	void expandAndSelectNode(const QVariant& id);

	LazyTreeNodePtr nodeFromItem(const QStandardItem* item) const;

signals:
	// Запрос данных: виджет-владелец должен загрузить детей и вызвать appendChildren
	void requestFetchChildren(const QVariant& parentId);

	// Пользователь активировал узел (двойной клик / Enter)
	void nodeActivated(const LazyTreeNodePtr& node, const QString& nodeName);

	// Изменился выделенный узел (пустой QVariant, если выделение снято)
	void selectionChanged(const LazyTreeNodePtr& node);

	// Клик по пустому месту
	void emptyAreaClicked();

protected:
	void mousePressEvent(QMouseEvent* event) override;

private slots:
	void onExpanded(const QModelIndex& index);
	void onNodeActivated(const QModelIndex& index);
	void onSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

private:
	class Private;
	std::unique_ptr<Private> d;
};
