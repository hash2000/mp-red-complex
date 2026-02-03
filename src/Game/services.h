#pragma once
#include <memory>

class EventBus;
class TimeService;
class WorldService;

class Services {
public:
	explicit Services(EventBus* eventBus);
	~Services();

	TimeService* timeService() const;

	WorldService* worldService() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
