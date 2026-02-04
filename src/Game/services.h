#pragma once
#include <memory>

class TimeService;
class WorldService;

class Services {
public:
	explicit Services();
	~Services();

	TimeService* timeService() const;

	WorldService* worldService() const;

	void run();

private:
	class Private;
	std::unique_ptr<Private> d;
};
