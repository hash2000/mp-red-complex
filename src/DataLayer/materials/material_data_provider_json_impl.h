#pragma once
#include "DataLayer/materials/i_material_data_provider.h"
#include <memory>

class Resources;

class MaterialsDataProviderJsonImpl : public IMaterialsDataProvider
{
public:
	MaterialsDataProviderJsonImpl(Resources* resources);
	~MaterialsDataProviderJsonImpl();
	bool loadIds(const QString& name, std::list<QString>& ids) const override;
	bool loadMaterial(const QString& name, Material& mat) const override;
	bool saveMaterial(Material& mat) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
