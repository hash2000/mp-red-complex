#include "DataFormat/data_format/Inventory/data_reader_json.h"
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <stdexcept>

namespace DataFormat::Inv {

	namespace {
		void fromJson(const QJsonObject& json, Inventory& inventory) {
			inventory.rows = json["rows"].toInt();
			inventory.cols = json["cols"].toInt();
			inventory.name = json["name"].toString();
			QJsonArray items = json["items"].toArray();
			for (const QJsonValue& it : items) {
				auto item = std::make_shared<InventoryItem>();

				item->entityId = it["entityId"]
					.toString()
					.toLower();
				item->id = it["id"]
					.toString()
					.toLower();
				
				item->x = it["x"].toInt();
				item->y = it["y"].toInt();
				item->count = it["count"].toInt(1);

				inventory.items.emplace(item->id, item);
			}
		}
	}

	DataReaderJson::DataReaderJson(DataStream& stream)
		: _stream(stream) {
	}

	void DataReaderJson::read(Inventory& result) {
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
