#include "Game/services/world_service.h"

class WorldService::Private {
public:
	Private(WorldService* parent)
	: q(parent) {
	}

	WorldService* q;
};


WorldService::WorldService(EventBus* eventBus, QObject* parent)
	: d(std::make_unique<Private>(this)) {

}

WorldService::~WorldService() = default;
