#include "DataFormat/game/entity.h"

Entity::Entity(uint32_t fid)
: _frameId(fid) {

}

uint32_t Entity::getFrameId() const {
  return _frameId;
}
