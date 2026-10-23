#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"
#include <unordered_map>
#include <list>

class LazyTreeBuilder {
public:
	/**
	 * Преобразует плоский список узлов в древовидную структуру
	 * @param flatNodes Плоский список узлов с заполненными id, parent_id, name, has_children, expanded
	 * @return Список корневых узлов (у которых parent_id == null) с заполненными children
	 */
	static LazyTreeNodeList buildFromFlat(const LazyTreeNodeList& flatNodes);
};
