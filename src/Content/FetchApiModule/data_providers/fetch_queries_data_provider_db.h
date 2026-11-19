#pragma once
#include "Content/DatabaseModule/data_providers/lazy_nodes_data_provider_db.h"

/// Поставщик дерева сохранённых запросов модуля FetchApi.
///
/// Сервис не является поставщиком данных: провайдер создаётся фабрикой
/// сервисов и передаётся FetchQueriesService/виджету.
class FetchQueriesDataProviderDb final : public LazyNodesDataProviderDb {
public:
	explicit FetchQueriesDataProviderDb(DatabasesService* databasesService);
	~FetchQueriesDataProviderDb() override;
};
