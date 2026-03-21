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
	case EquipmentSlotType::Gloves: return "Gloves";
	case EquipmentSlotType::Amulet: return "Amulet";
	case EquipmentSlotType::Boots: return "Boots";
	case EquipmentSlotType::Ring1: return "Ring1";
	case EquipmentSlotType::Ring2: return "Ring2";
	case EquipmentSlotType::Ring3: return "Ring3";
	case EquipmentSlotType::Backpack: return "Backpack";
	case EquipmentSlotType::Bag1: return "Bag1";
	case EquipmentSlotType::Bag2: return "Bag2";
	default: return "";
	}
}

bool EquipmentDataWriterJsonImpl::saveEquipment(const QUuid& id, const Equipment& equipment) const {
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
	const auto path = QString("data/equipment/");
	auto dataDir = d->resources->Variables.get("Resources.Path", "")
		.toString();
	if (dataDir.isEmpty()) {
		qWarning() << "EquipmentDataWriterJsonImpl: Resources.Path not found in variables";
		return false;
	}

	QDir dir(dataDir);
	if (!dir.exists(path)) {
		dir.mkpath(path);
	}

	const auto fileName = id.toString(QUuid::StringFormat::WithoutBraces).toLower() + ".json";
	QFile file(dir.filePath(path + fileName));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "EquipmentDataWriterJsonImpl: can't open file for writing:" << file.fileName();
		return false;
	}

	file.write(doc.toJson());
	return true;
}
