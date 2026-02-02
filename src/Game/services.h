#pragma once
#include <memory>

class TimeService;
class EventBus;

class Services {
public:
	explicit Services(EventBus* eventBus);
	~Services();

	TimeService* timeService() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
