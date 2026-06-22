#pragma once
#include <QUuid>
#include <memory>
#include <list>

class Container;
class ContainerItem;

class IContainerDataProvider {
public:
	virtual ~IContainerDataProvider() = default;

	virtual std::list<std::shared_ptr<Container>> containers(const QUuid& id) = 0;
	virtual bool save(const QUuid& id, const Container& cont) = 0;
};
