#pragma once
#include "Content/DatabaseModule/data_providers/lazy_nodes_data_provider_db.h"

/// Поставщик узлов системного маршрутизатора.
///
/// Создаёт `SystemRouterDataNode`, который решает, какие значения свойств
/// хранить в `data_blobs`. Таблица nodes имеет типовую структуру
/// id/parent_id/name/data.
class SystemRouterDataProviderDb final : public LazyNodesDataProviderDb {
public:
	explicit SystemRouterDataProviderDb(DatabasesService* databasesService);
	~SystemRouterDataProviderDb() override;

	LazyTreeNodePtr createTreeNode() override;
	LazyTreeNodePtr createRootNode() override;
};
