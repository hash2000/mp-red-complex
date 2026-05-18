#pragma once
#include "DataLayer/materials/i_material_data_provider.h"
#include <memory>

class Resources;

class MaterialsDataProviderJsonImpl : public IMaterialsDataProvider
{
public:
	MaterialsDataProviderJsonImpl(Resources* resources);
	~MaterialsDataProviderJsonImpl();
	bool loadIds(const QString& name, std::list<QString>& ids) override;
	bool loadMaterial(const QString& name, Material& mat) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
