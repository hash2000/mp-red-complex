#pragma once
#include "Resources/resources.h"

class IDataSerializer {
public:
	virtual void serialize(Resources* resources) = 0;
	virtual void deserialize(Resources* resources) = 0;
};
