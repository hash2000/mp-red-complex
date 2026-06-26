#include "Libs/DataStream/format/json/data_reader.h"
#include "Libs/DataStream/data_stream.h"
#include "Libs/Resources/resources.h"

namespace Format::Json {

	DataReader::DataReader(Resources* resources, const QString& container, const QString& path)
		: BaseDataReader(resources, container, path, "Format::Json::DataReader") {
	}

	bool DataReader::read(QJsonObject& result) {
		if (!_block) {
			return false;
		}

		QByteArray buffer;
		buffer.reserve(_block->size());
		while (_block->remains() > 0) {
			buffer.append(static_cast<char>(_block->u8()));
		}

		QJsonParseError parseError;
		auto document = QJsonDocument::fromJson(buffer, &parseError);
		if (parseError.error != QJsonParseError::NoError) {
			qWarning() << QString("Load json [%1], position [%2]. %3")
				.arg(_block->name())
				.arg(_block->position())
				.arg(parseError.errorString())
				.toStdString();
			return false;
		}

		result = document.object();
		return true;
	}
}
