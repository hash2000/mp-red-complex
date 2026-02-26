#include "DataLayer/equipment/equipment_data_provider_json_impl.h"
#include "DataLayer/equipment/equipment.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"

class EquipmentDataProviderJsonImpl::Private {
public:
	Private(EquipmentDataProviderJsonImpl* parent)
		: q(parent) {
	}

	EquipmentDataProviderJsonImpl* q;
	Resources* resources;
};

EquipmentDataProviderJsonImpl::EquipmentDataProviderJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

EquipmentDataProviderJsonImpl::~EquipmentDataProviderJsonImpl() = default;

bool EquipmentDataProviderJsonImpl::loadEquipment(const QUuid& id, Equipment& equipment) {
	const auto path = QString("equipment/%1.json")
		.arg(id.toString(QUuid::StringFormat::WithoutBraces).toLower());

	auto stream = d->resources->getStream("data", path);
	if (!stream) {
		return false;
	}

	QJsonObject json;
	Format::Json::DataReader reader(d->resources, "data", path);
	if (!reader.read(json)) {
		return false;
	}
}
