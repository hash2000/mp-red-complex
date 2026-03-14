#include "DataLayer/equipment/equipment_data_writer_json_impl.h"
#include "DataLayer/equipment/equipment.h"
#include "DataLayer/equipment/equipment_slot.h"
#include "Resources/resources.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

class EquipmentDataWriterJsonImpl::Private {
public:
	Private(EquipmentDataWriterJsonImpl* parent)
		: q(parent) {
	}

	EquipmentDataWriterJsonImpl* q;
	Resources* resources;
};

EquipmentDataWriterJsonImpl::EquipmentDataWriterJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

EquipmentDataWriterJsonImpl::~EquipmentDataWriterJsonImpl() = default;

QString slotToString(EquipmentSlotType slot) {
	switch (slot) {
	case EquipmentSlotType::Head: return "Head";
	case EquipmentSlotType::Body: return "Body";
	case EquipmentSlotType::WeaponLeft: return "WeaponLeft";
	case EquipmentSlotType::WeaponRight: return "WeaponRight";
	case EquipmentSlotType::GlovesLeft: return "GlovesLeft";
	case EquipmentSlotType::GlovesRight: return "GlovesRight";
	case EquipmentSlotType::Boots: return "Boots";
	case EquipmentSlotType::RingLeft: return "RingLeft";
	case EquipmentSlotType::RingRight: return "RingRight";
	case EquipmentSlotType::Amulet: return "Amulet";
	case EquipmentSlotType::Backpack: return "Backpack";
	case EquipmentSlotType::Bag1: return "Bag1";
	case EquipmentSlotType::Bag2: return "Bag2";
	default: return "";
	}
}

bool EquipmentDataWriterJsonImpl::saveEquipment(const QUuid& id, const Equipment& equipment) const {
	const auto path = QString("data/equipment/%1.json")
		.arg(id.toString(QUuid::StringFormat::WithoutBraces).toLower());

	QJsonObject json;
	json["id"] = equipment.id
		.toString(QUuid::StringFormat::WithoutBraces)
		.toLower();

	QJsonArray itemsArr;
	for (const auto& item : equipment.items) {
		QJsonObject itemObj;
		itemObj["id"] = item.id
			.toString(QUuid::StringFormat::WithoutBraces)
			.toLower();
		itemObj["slot"] = slotToString(item.slot);
		itemsArr.append(itemObj);
	}
	json["items"] = itemsArr;

	QJsonDocument doc(json);

	// Получаем путь к директории data из Resources
	auto dataDir = d->resources->Variables.get("Resources.Path", "")
		.toString();
	if (dataDir.isEmpty()) {
		qWarning() << "EquipmentDataWriterJsonImpl: Resources.Path not found in variables";
		return false;
	}

	QDir dir(dataDir);
	if (!dir.exists("equipment")) {
		dir.mkpath("equipment");
	}

	QFile file(dir.filePath("data/equipment/" + id.toString(QUuid::StringFormat::WithoutBraces).toLower() + ".json"));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "EquipmentDataWriterJsonImpl: can't open file for writing:" << file.fileName();
		return false;
	}

	file.write(doc.toJson());
	return true;
}
