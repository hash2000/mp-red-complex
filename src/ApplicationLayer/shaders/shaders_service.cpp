#include "ApplicationLayer/shaders/shaders_service.h"
#include "DataLayer/shaders/i_shaders_data_provider.h"

class ShadersService::Private {
public:
	Private(ShadersService* parent) : q(parent) {}

	ShadersService* q;
	IShadersDataProvider* shadersDataProvider = nullptr;
};

ShadersService::ShadersService(IShadersDataProvider* shadersDataProvider)
	: d(std::make_unique<Private>(this)) {
	d->shadersDataProvider = shadersDataProvider;
}

ShadersService::~ShadersService() = default;


