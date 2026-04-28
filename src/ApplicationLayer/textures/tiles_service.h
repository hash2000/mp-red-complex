#pragma once
#include <memory>

class TilesService {
public:
	TilesService();
	~TilesService();

private:
	class Private;
	std::unique_ptr<Private> d;
};
