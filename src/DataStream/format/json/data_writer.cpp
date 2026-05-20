#include "DataStream/format/json/data_writer.h"
#include "DataStream/data_write_stream.h"
#include "Resources/resources.h"

namespace Format::Json {

	DataWriter::DataWriter(Resources* resources, const QString& container, const QString& path)
		: BaseDataWriter(resources, container, path, "Format::Json::DataReader") {
	}

	bool DataWriter::write(const QJsonObject& buffer) {
		if (!_block) {
			return false;
		}

		//QByteArray buffer;
		//buffer.reserve(_block->size());
		//while (_block->remains() > 0) {
		//	buffer.append(static_cast<char>(_block->u8()));
		//}

		//QJsonParseError parseError;
		//auto document = QJsonDocument::fromJson(buffer, &parseError);
		//if (parseError.error != QJsonParseError::NoError) {
		//	throw std::runtime_error(QString("Load json [%1], position [%2]. %3")
		//		.arg(_block->name())
		//		.arg(_block->position())
		//		.arg(parseError.errorString())
		//		.toStdString());
		//}

		//result = document.object();
		return true;
	}
}
