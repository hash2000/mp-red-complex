#pragma once
#include <memory>

class ServicesRegistry;
class Resources;

class Services {
public:
	explicit Services(Resources* resources);
	~Services();

	std::unique_ptr<ServicesRegistry> create();

private:
	class Private;
	std::unique_ptr<Private> d;
};

