#include "DataLayer/materials/material_data_provider_json_impl.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"
#include <QJsonObject>
#include <QJsonArray>

class MaterialsDataProviderJsonImpl::Private {
public:
	Private(MaterialsDataProviderJsonImpl* parent) : q(parent) { }
	MaterialsDataProviderJsonImpl* q;
	Resources* resources;
};


MaterialsDataProviderJsonImpl::MaterialsDataProviderJsonImpl(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

MaterialsDataProviderJsonImpl::~MaterialsDataProviderJsonImpl() = default;

bool MaterialsDataProviderJsonImpl::loadIds(const QString& name, std::list<QString>& ids) {
	const auto path = QString("materials/lists/%1.json")
		.arg(name);

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "assets", path);
	if (!reader.read(json)) {
		return false;
	}

	const QJsonArray items = json["items"]
		.toArray();

	for (const auto& item : items) {
		ids.push_back(item.toString());
	}

	return true;
} 

bool MaterialsDataProviderJsonImpl::loadMaterial(const QString& name, Material& mat) {

	return true;
}
