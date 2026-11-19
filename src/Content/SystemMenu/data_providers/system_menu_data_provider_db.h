#pragma once
#include "Content/DatabaseModule/data_providers/lazy_nodes_data_provider_db.h"

/// Поставщик узлов системного меню.
///
/// Отдельный поставщик (не сервис): создаётся фабрикой сервисов и передаётся
/// SystemMenuService. Таблица menu_items имеет типовую структуру
/// id/parent_id/name/data, поэтому используется общий LazyNodesDataProviderDb.
class SystemMenuDataProviderDb final : public LazyNodesDataProviderDb {
public:
	explicit SystemMenuDataProviderDb(DatabasesService* databasesService);
	~SystemMenuDataProviderDb() override;
};
