#include "Content/MaterialsModule/data_providers/material_data_provider_json_impl.h"
#include "Libs/DataStream/format/json/data_reader.h"
#include "Libs/DataStream/format/json/data_writer.h"
#include "Libs/Resources/resources.h"
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

bool MaterialsDataProviderJsonImpl::loadIds(const QString& name, std::list<QString>& ids) const {
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

bool MaterialsDataProviderJsonImpl::loadMaterial(const QString& name, Material& mat) const {
	const auto path = QString("materials/%1.json")
		.arg(name);

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "assets", path);
	if (!reader.read(json)) {
		return false;
	}

	if (json.contains("albedo") && json["albedo"].isObject()) {
		QJsonObject albedo = json["albedo"].toObject();
		mat.baseColor = QVector4D(
			albedo["x"].toDouble(),
			albedo["y"].toDouble(),
			albedo["z"].toDouble(),
			albedo["w"].toDouble());
	}

	mat.name = name;

	return true;
}

bool MaterialsDataProviderJsonImpl::saveMaterial(Material& mat) const {
	const auto path = QString("materials/%1.json")
		.arg(mat.name);

	QJsonObject albedo;
	albedo["x"] = mat.baseColor.x();
	albedo["y"] = mat.baseColor.y();
	albedo["z"] = mat.baseColor.z();
	albedo["w"] = mat.baseColor.w();

	QJsonObject json;
	json["albedo"] = albedo;



	Format::Json::DataWriter reader(d->resources, "assets", path);
	if (!reader.write(json)) {
		return false;
	}

	return true;
}
