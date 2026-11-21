#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_data_node.h"

#include <QHash>
#include <QList>
#include <QPair>
#include <QStringList>
#include <QVariantMap>

#include <algorithm>
#include <memory>

/// Узел для тестов: обычная реализация из библиотеки.
inline LazyTreeNodePtr makeLazyTestNode(const QVariant& id, const QVariant& parentId,
	const QString& name) {
	auto node = std::make_shared<LazyTreeDataNode>();
	node->setId(id);
	node->setParentId(parentId);
	node->setName(name);
	return node;
}

/// Пометить узел как ленивый: дети есть, но загружаются отдельным запросом.
inline void markLazyTestNode(const LazyTreeNodePtr& node) {
	node->setChildren(LazyTreeNodeList());
}

/// Пометить узел как лист: детей нет.
inline void markLeafTestNode(const LazyTreeNodePtr& node) {
	node->clearChildren();
}

/// Поставщик данных в памяти для тестов дерева.
class MockLazyTreeProvider : public ILazyNodesDataProvider {
public:
	mutable QList<QVariant> getCalls;
	mutable QList<QString> searchCalls;

	QList<LazyTreeNodePtr> addedNodes;
	QList<LazyTreeNodePtr> updatedNodes;
	QList<QVariant> removedIds;
	QList<QPair<QVariant, bool>> expandedCalls;

	bool addResult = true;
	bool updateResult = true;
	bool removeResult = true;
	bool expandedResult = true;
	bool failGet = false;
	bool failSearch = false;

	mutable QString lastErrorValue;

	LazyTreeNodeList searchResult;

	LazyTreeNodePtr createTreeNode() override {
		return std::make_shared<LazyTreeDataNode>();
	}

	LazyTreeNodePtr createRootNode() override {
		return std::make_shared<LazyTreeDataNode>();
	}

	LazyTreeNodeList get(const QVariant& parentId) const override {
		getCalls.append(parentId);
		lastErrorValue.clear();
		if (failGet) {
			lastErrorValue = QStringLiteral("Не удалось загрузить детей");
			return {};
		}

		return _children.value(key(parentId));
	}

	LazyTreeNodePtr getNode(const QVariant& id) const override {
		return _nodes.value(key(id));
	}

	LazyTreeNodeList search(const QString& text) const override {
		searchCalls.append(text);
		lastErrorValue.clear();
		if (failSearch) {
			lastErrorValue = QStringLiteral("Не удалось выполнить поиск");
			return {};
		}

		return searchResult;
	}

	bool add(const LazyTreeNodePtr& node) override {
		lastErrorValue.clear();
		if (!addResult) {
			lastErrorValue = QStringLiteral("Не удалось добавить узел");
			return false;
		}

		addedNodes.append(node);
		node->setId(QVariant::fromValue<qint64>(1000 + addedNodes.size()));
		_nodes.insert(key(node->id()), node);
		_children[key(node->parentId())].push_back(node);
		return true;
	}

	bool update(const LazyTreeNodePtr& node) override {
		updatedNodes.append(node);
		lastErrorValue.clear();
		if (!updateResult) {
			lastErrorValue = QStringLiteral("Не удалось сохранить узел");
			return false;
		}

		_nodes.insert(key(node->id()), node);
		return true;
	}

	bool setExpanded(const QVariant& id, bool expanded) override {
		expandedCalls.append({ id, expanded });
		lastErrorValue.clear();
		if (!expandedResult) {
			lastErrorValue = QStringLiteral("Не удалось сохранить признак раскрытия");
			return false;
		}

		if (const auto node = _nodes.value(key(id))) {
			node->setExpanded(expanded);
		}

		return true;
	}

	bool remove(const QVariant& id) override {
		removedIds.append(id);
		lastErrorValue.clear();
		if (!removeResult) {
			lastErrorValue = QStringLiteral("Не удалось удалить узел");
			return false;
		}

		_nodes.remove(key(id));
		for (auto it = _children.begin(); it != _children.end(); it++) {
			LazyTreeNodeList& list = it.value();
			list.erase(std::remove_if(list.begin(), list.end(),
				[&id](const LazyTreeNodePtr& node) {
					return !node || node->id() == id;
				}), list.end());
		}
		return true;
	}

	QString lastError() const override {
		return lastErrorValue;
	}

	void resetError() {
		lastErrorValue.clear();
	}

	void setChildren(const QVariant& parentId, const LazyTreeNodeList& nodes) {
		_children.insert(key(parentId), nodes);
	}

	void registerNode(const LazyTreeNodePtr& node) {
		_nodes.insert(key(node->id()), node);
	}

	void registerNodes(const LazyTreeNodeList& nodes) {
		for (const auto& node : nodes) {
			registerNode(node);
		}
	}

private:
	static QString key(const QVariant& value) {
		if (!value.isValid() || value.isNull()) {
			return QStringLiteral("root");
		}

		return QStringLiteral("%1:%2").arg(value.typeId()).arg(value.toString());
	}

	QHash<QString, LazyTreeNodeList> _children;
	QHash<QString, LazyTreeNodePtr> _nodes;
};
