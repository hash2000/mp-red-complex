#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

/// Построение иерархии из плоского списка узлов.
///
/// Используется, когда поставщик данных получает плоский список с корректными
/// id/parent_id (например, SQL-выборка с рекурсивным CTE).
class LazyTreeBuilder {
public:
	/// Преобразует плоский список в древовидную структуру.
	///
	/// Узлы с пустым/неизвестным parent_id, а также узлы, образующие цикл,
	/// становятся корневыми — так данные не теряются. Порядок корневых узлов
	/// и соседей соответствует порядку во входном списке.
	static LazyTreeNodeList buildFromFlat(const LazyTreeNodeList& flatNodes);
};
