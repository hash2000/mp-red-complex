#include "DataStream/format/json/data_reader.h"
#include "DataStream/data_stream.h"
#include "Resources/resources.h"

namespace Format::Json {

	DataReader::DataReader(Resources* resources, const QString& container, const QString& path) {
		auto stream = resources->getStream(container, path);
		if (!stream.has_value()) {
			qFatal() << QString("Format::Json::DataReader: Error read %1, read %2.")
				.arg(container)
				.arg(path);
			return;
		}

		_block = stream.value()->makeBlockAsStream();
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
			throw std::runtime_error(QString("Load json [%1], position [%2]. %3")
				.arg(_block->name())
				.arg(_block->position())
				.arg(parseError.errorString())
				.toStdString());
		}

		result = document.object();
		return true;
	}
}
