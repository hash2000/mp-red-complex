#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_data_node.h"

/// Узел SystemRouter: решает, какие значения свойств хранить в data_blobs.
class SystemRouterDataNode final : public LazyTreeDataNode {
public:
	SystemRouterDataNode() = default;
	~SystemRouterDataNode() override = default;

	bool propertyValueIsBlob(const QString& name) const override;
};
