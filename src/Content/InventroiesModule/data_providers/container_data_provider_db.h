#pragma once 
#include "Content/InventroiesModule/data_providers/i_container_data_provider.h"
#include <memory>
#include <list>

class DatabasesService;

class ContainerDataProvider : public IContainerDataProvider {
public:
	ContainerDataProvider(DatabasesService* databasesService);
	~ContainerDataProvider() override;

	std::shared_ptr<Container> container(const QUuid& id) override;
	bool save(const QUuid& id, const Container& cont) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
