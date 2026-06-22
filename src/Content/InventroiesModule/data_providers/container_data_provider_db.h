#pragma once 
#include "Content/InventroiesModule/data_providers/i_container_data_provider.h"
#include <memory>

class DatabasesService;

class ContainerDataProvider : public IContainerDataProvider {
public:
	ContainerDataProvider(DatabasesService* databasesService);
	~ContainerDataProvider() override;

	std::list<std::shared_ptr<Container>> containers(const QUuid& id) override;
	bool save(const QUuid& id, const Container& cont) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
