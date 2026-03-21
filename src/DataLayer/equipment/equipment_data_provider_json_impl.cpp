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

	equipment.id = id;

	const QJsonArray items = json["items"]
		.toArray();

	for (const QJsonValue& item : items) {
		EquipmentItem eqItem;
		eqItem.id = QUuid::fromString(item["id"].toString());
		const auto slot = item["slot"].toString();
		if (slot == "Head") eqItem.slot = EquipmentSlotType::Head;
		else if (slot == "Body") eqItem.slot = EquipmentSlotType::Body;
		else if (slot == "WeaponLeft") eqItem.slot = EquipmentSlotType::WeaponLeft;
		else if (slot == "Gloves") eqItem.slot = EquipmentSlotType::Gloves;
		else if (slot == "Amulet") eqItem.slot = EquipmentSlotType::Amulet;
		else if (slot == "Boots") eqItem.slot = EquipmentSlotType::Boots;
		else if (slot == "Ring1") eqItem.slot = EquipmentSlotType::Ring1;
		else if (slot == "Ring2") eqItem.slot = EquipmentSlotType::Ring2;
		else if (slot == "Ring3") eqItem.slot = EquipmentSlotType::Ring3;
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
