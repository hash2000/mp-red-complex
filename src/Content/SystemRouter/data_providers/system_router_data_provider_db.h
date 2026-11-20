#pragma once
#include "Content/DatabaseModule/data_providers/lazy_nodes_data_provider_db.h"

/// Поставщик узлов системного маршрутизатора.
///
/// Отдельный поставщик (не сервис): создаётся фабрикой сервисов и передаётся
/// SystemRouterService. Таблица nodes имеет типовую структуру
/// id/parent_id/name/data, поэтому используется общий LazyNodesDataProviderDb.
class SystemRouterDataProviderDb final : public LazyNodesDataProviderDb {
public:
	explicit SystemRouterDataProviderDb(DatabasesService* databasesService);
	~SystemRouterDataProviderDb() override;
};
