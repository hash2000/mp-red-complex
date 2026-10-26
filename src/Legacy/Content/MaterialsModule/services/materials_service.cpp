#include "Content/MaterialsModule/services/materials_service.h"
#include "Content/MaterialsModule/services/material_mime_data.h"
#include "Content/MaterialsModule/data_providers/i_material_data_provider.h"

#include <map>

class MaterialsService::Private {
public:
	Private(MaterialsService* parent) : q(parent) {}
	MaterialsService* q;

	IMaterialsDataProvider* materialsDataProvider;
	std::map<QString, std::unique_ptr<MaterialMimeData>> materials;
};

MaterialsService::MaterialsService(IMaterialsDataProvider* materialsDataProvider, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->materialsDataProvider = materialsDataProvider;
}

MaterialsService::~MaterialsService() = default;

MaterialMimeData* MaterialsService::loadMaterial(const QString& name) {
	const auto& it = d->materials.find(name);
	if (it != d->materials.end()) {
		return it->second.get();
	}

	Material mat;
	if (!d->materialsDataProvider->loadMaterial(name, mat)) {
		return nullptr;
	}

	auto item = d->materials.emplace(name, std::make_unique<MaterialMimeData>());
	auto material = item.first->second.get();
	material->name = mat.name;
	material->baseColor = mat.baseColor;
	material->metallic = mat.metallic;
	material->roughness = mat.roughness;
	material->normalScale = mat.normalScale;
	material->occlusionStrength = mat.occlusionStrength;
	material->emissive = mat.emissive;
	material->alphaCutoff = mat.alphaCutoff;

	return material;
}
