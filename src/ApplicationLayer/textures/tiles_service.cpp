#include "ApplicationLayer/textures/tiles_service.h"


class TilesService::Private {
public:
	Private(TilesService* parent): q(parent) {}

	TilesService* q;

};

TilesService::TilesService()
: d(std::make_unique<Private>(this)) {
}

TilesService::~TilesService() = default;
