#include "DataFormat/data_format/item/data_reader_json.h"
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <stdexcept>

namespace DataFormat::Item {

	namespace {
		void fromJson(const QJsonObject& json, InventoryItem& item) {
			item.entityId = json["id"].toString();
			item.name = json["name"].toString();
			item.description = json["description"].toString();
			item.iconPath = json["icon"].toString();

			// Тип предмета
			QString typeStr = json["type"].toString().toLower();
			if (typeStr == "equipment") {
				item.type = InventoryItemType::Equipment;
				QString equipType = json["equipmentType"].toString().toLower();
				if (equipType == "head") item.equipmentType = EquipmentItemType::Head;
				else if (equipType == "body") item.equipmentType = EquipmentItemType::Body;
				else if (equipType == "weapon") item.equipmentType = EquipmentItemType::Weapon;
				else if (equipType == "shield") item.equipmentType = EquipmentItemType::Shield;
				else if (equipType == "gloves") item.equipmentType = EquipmentItemType::Gloves;
				else if (equipType == "boots") item.equipmentType = EquipmentItemType::Boots;
				else if (equipType == "ring") item.equipmentType = EquipmentItemType::Ring;
				else if (equipType == "amulet") item.equipmentType = EquipmentItemType::Amulet;
			}
			else if (typeStr == "resource") {
				item.type = InventoryItemType::Resource;
				item.maxStack = json["maxStack"].toInt(100);
			}
			else if (typeStr == "component") {
				item.type = InventoryItemType::Component;
				item.maxStack = json["maxStack"].toInt(100);
			}
			else if (typeStr == "container") {
				item.type = InventoryItemType::Container;
			}

			// Размер в ячейках
			item.width = json["width"].toInt(1);
			item.height = json["height"].toInt(1);

			// Рецепт крафта (опционально)
			if (json.contains("recipe")) {
				item.recipe = InventoryItemRecipe();
				QJsonArray ingredients = json["recipe"]
					.toObject()["ingredients"]
					.toArray();

				for (const QJsonValue& ingVal : ingredients) {
					QJsonObject ingObj = ingVal.toObject();
					item.recipe->ingredients.push_back({
							ingObj["itemId"].toString(),
							ingObj["amount"].toInt()
						});
				}
			}
		}
	}

	DataReaderJson::DataReaderJson(DataStream& stream)
		: _stream(stream) {

	}

	void DataReaderJson::read(InventoryItem& result) {
		QByteArray buffer;
		buffer.reserve(_stream.size());
		while (_stream.remains() > 0) {
			buffer.append(static_cast<char>(_stream.u8()));
		}

		QJsonParseError parseError;
		auto document = QJsonDocument::fromJson(buffer, &parseError);
		if (parseError.error != QJsonParseError::NoError) {
			throw std::runtime_error(QString("Load json [%1], position [%2]. %3")
				.arg(_stream.name())
				.arg(_stream.position())
				.arg(parseError.errorString())
				.toStdString());
		}

		const auto jobject = document.object();
		fromJson(jobject, result);
	}

}
