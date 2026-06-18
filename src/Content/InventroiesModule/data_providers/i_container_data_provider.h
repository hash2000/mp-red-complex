#pragma once
#include <QUuid>
#include <memory>

class Container;
class ContainerItem;

class IContainerDataProvider {
public:
	virtual ~IContainerDataProvider() = default;

	virtual std::shared_ptr<Container> container(const QUuid& id) = 0;
	virtual bool save(const QUuid& id, const Container& cont) = 0;
};
