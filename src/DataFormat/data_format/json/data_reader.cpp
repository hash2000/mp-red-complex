#include "DataFormat/data_format/json/data_reader.h"
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <stdexcept>

namespace DataFormat::Json {

	DataReader::DataReader(DataStream& stream)
		: _stream(stream) {

	}

	void DataReader::read(QJsonObject& result) {
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

		result = document.object();
	}

}
