#include "ApplicationLayer/materials/materials_service.h"

class MaterialsService::Private {
public:
	Private(MaterialsService* parent) : q(parent) {}
	MaterialsService* q;
};

MaterialsService::MaterialsService(IMaterialsDataProvider* materialsDataProvider, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
}

MaterialsService::~MaterialsService() = default;
