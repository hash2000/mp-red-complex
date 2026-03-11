#include "DataLayer/equipment/equipment_data_provider_json_impl.h"
#include "DataLayer/equipment/equipment.h"
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"
#include <QJsonArray>

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

	equipment.id = id
		.toString(QUuid::StringFormat::WithoutBraces)
		.toLower();

	const QJsonArray items = json["items"]
		.toArray();

	for (const QJsonValue& item : items) {
		EquipmentItem eqItem;
		eqItem.id = item["id"].toString();
		const auto slot = item["slot"].toString();
		if (slot == "Head") eqItem.slot = EquipmentSlotType::Head;
		else if (slot == "Body") eqItem.slot = EquipmentSlotType::Body;
		else if (slot == "WeaponLeft") eqItem.slot = EquipmentSlotType::WeaponLeft;
		else if (slot == "GlovesLeft") eqItem.slot = EquipmentSlotType::GlovesLeft;
		else if (slot == "GlovesRight") eqItem.slot = EquipmentSlotType::GlovesRight;
		else if (slot == "Boots") eqItem.slot = EquipmentSlotType::Boots;
		else if (slot == "RingLeft") eqItem.slot = EquipmentSlotType::RingLeft;
		else if (slot == "Amulet") eqItem.slot = EquipmentSlotType::Amulet;
		else if (slot == "Backpack") eqItem.slot = EquipmentSlotType::Backpack;
		else if (slot == "Bag1") eqItem.slot = EquipmentSlotType::Bag1;
		else if (slot == "Bag2") eqItem.slot = EquipmentSlotType::Bag2;
		else {
			qWarning() << "loadEquipment: equipment from" << equipment.id << "item" << eqItem.id << "unknown slot" << slot;
			continue;
		}

		equipment.items.push_back(eqItem);
	}

}
