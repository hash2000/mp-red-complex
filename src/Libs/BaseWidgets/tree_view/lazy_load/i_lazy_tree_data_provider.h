#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <QString>
#include <QVariant>

/// Поставщик данных для LazyTreeView.
///
/// Интерфейс намеренно не наследует QObject: реализации могут быть
/// синхронными (база данных, кэш) и асинхронными (HTTP/gRPC — тогда
/// результат приходит через сигналы конкретной реализации).
class ILazyNodesDataProvider {
public:
	virtual ~ILazyNodesDataProvider() = default;

	/// Создать пустой узел для добавления в дерево.
	virtual LazyTreeNodePtr createTreeNode() = 0;

	/// Создать корневой узел (скрытый владелец верхнего уровня).
	virtual LazyTreeNodePtr createRootNode() = 0;

	/// Дочерние узлы parentId. Пустой/null parentId — корневые узлы.
	virtual LazyTreeNodeList get(const QVariant& parentId) const = 0;

	/// Узел по идентификатору (nullptr, если не найден).
	virtual LazyTreeNodePtr getNode(const QVariant& id) const {
		Q_UNUSED(id);
		return nullptr;
	}

	virtual LazyTreeNodeList search(const QString& text) const = 0;
	virtual bool add(const LazyTreeNodePtr& node) = 0;
	virtual bool remove(const QVariant& id) = 0;
	virtual bool update(const LazyTreeNodePtr& node) = 0;

	/// Описание последней ошибки. Пустая строка — ошибки не было.
	/// Нужна, чтобы отличить «детей нет» от «запрос не выполнен».
	virtual QString lastError() const {
		return {};
	}
};
