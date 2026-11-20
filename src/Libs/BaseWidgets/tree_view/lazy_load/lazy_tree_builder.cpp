#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_builder.h"

#include <QDebug>
#include <QHash>
#include <QMetaType>
#include <QVariant>
#include <QSet>

namespace {

/// Канонический ключ варианта.
///
/// QVariant(1) и QVariant(QString("1")) не должны попадать в одну запись,
/// а числовые типы (int/qint64) должны считаться одним идентификатором.
QString nodeKey(const QVariant& value) {
	if (!value.isValid() || value.isNull()) {
		return QStringLiteral("null");
	}

	switch (value.typeId()) {
	case QMetaType::Int:
	case QMetaType::UInt:
	case QMetaType::Long:
	case QMetaType::ULong:
	case QMetaType::LongLong:
	case QMetaType::ULongLong:
		return QStringLiteral("n:%1").arg(value.toLongLong());
	default:
		return QStringLiteral("s:%1:%2").arg(value.typeId()).arg(value.toString());
	}
}

bool isRootParent(const QVariant& parentId) {
	return !parentId.isValid() || parentId.isNull();
}

/// Проверка, что цепочка родителей не возвращается к самому узлу.
bool formsCycle(const LazyTreeNodePtr& node, const QHash<QString, LazyTreeNodePtr>& nodeMap) {
	QSet<QString> visited;
	QString currentKey = nodeKey(node->id());
	visited.insert(currentKey);

	auto parentId = node->parentId();
	while (!isRootParent(parentId)) {
		const QString parentKey = nodeKey(parentId);
		if (visited.contains(parentKey)) {
			return true;
		}

		visited.insert(parentKey);

		const auto parentIt = nodeMap.constFind(parentKey);
		if (parentIt == nodeMap.constEnd()) {
			// Родителя нет во входном списке — узел станет корневым.
			return false;
		}

		parentId = parentIt.value()->parentId();
	}

	return false;
}

} // namespace

LazyTreeNodeList LazyTreeBuilder::buildFromFlat(const LazyTreeNodeList& flatNodes) {
	LazyTreeNodeList roots;

	QHash<QString, LazyTreeNodePtr> nodeMap;
	nodeMap.reserve(static_cast<qsizetype>(flatNodes.size()));

	// Первый проход: индексируем узлы по идентификатору.
	for (const auto& node : flatNodes) {
		if (!node || isRootParent(node->id())) {
			continue;
		}

		nodeMap.insert(nodeKey(node->id()), node);
	}

	// Второй проход: раскладываем узлы по родителям.
	for (const auto& node : flatNodes) {
		if (!node) {
			continue;
		}

		const auto parentId = node->parentId();
		if (isRootParent(parentId)) {
			roots.push_back(node);
			continue;
		}

		const auto parentIt = nodeMap.constFind(nodeKey(parentId));
		if (parentIt == nodeMap.constEnd() || parentIt.value() == node) {
			roots.push_back(node);
			continue;
		}

		if (formsCycle(node, nodeMap)) {
			// Данные некорректны: не строим цикл, оставляем узел корневым.
			qWarning() << "LazyTreeBuilder: cycle detected for node" << node->id();
			roots.push_back(node);
			continue;
		}

		parentIt.value()->appendChild(node);
	}

	return roots;
}
