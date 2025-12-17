#pragma once
#include "DataFormat/proto/pro.h"


class Entity {
public:
	Entity(uint32_t fid);

	virtual ~Entity() = default;

	virtual Proto::ObjectType type() const = 0;

	uint32_t getFrameId() const;

private:
	uint32_t _frameId;
};
