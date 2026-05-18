#pragma once
#include "DataLayer/materials/material.h"
#include <list>

class IMaterialsDataProvider {
public:
	virtual bool loadIds(const QString& name, std::list<QString>& ids) = 0;
	virtual bool loadMaterial(const QString& name, Material& mat) = 0;
};
