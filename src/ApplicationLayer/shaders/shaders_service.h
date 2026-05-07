#pragma once
#include <memory>

class IShadersDataProvider;

class ShadersService {
public:
	ShadersService(IShadersDataProvider* shadersDataProvider);
	~ShadersService();

private:
	class Private;
	std::unique_ptr<Private> d;
};
