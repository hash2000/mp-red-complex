#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_builder.h"
#include <QVariant>
#include <QHash>

struct VariantHash {
	std::size_t operator()(const QVariant& v) const {
		return std::hash<QString>{}(v.toString());
	}
};

LazyTreeNodeList LazyTreeBuilder::buildFromFlat(const LazyTreeNodeList& flatNodes) {
	LazyTreeNodeList roots;

	std::unordered_map<QVariant, LazyTreeNodePtr, VariantHash> nodeMap;
	nodeMap.reserve(flatNodes.size());

	// Первый проход: добавляем все узлы в map
	for (const auto& node : flatNodes) {
		if (node) {
			nodeMap[node->id()] = node;
		}
	}

	// Второй проход: строим дерево
	for (const auto& node : flatNodes) {
		if (!node) {
			continue;
		}

		auto parentId = node->parentId();
		if (parentId.isNull()) {
			roots.push_back(node);
		}
		else {
			auto parentIt = nodeMap.find(parentId);
			if (parentIt != nodeMap.end()) {
				parentIt->second->appendChild(node);
			}
		}
	}

	return roots;
}
