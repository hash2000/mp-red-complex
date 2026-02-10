#include "DataFormat/data_format/pixmap/data_reader.h"

namespace DataFormat::Pixmap {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
	}

	void DataReader::read(QPixmap& result) {
		QByteArray buffer;
		buffer.reserve(_stream.size());
		while (_stream.remains() > 0) {
			buffer.append(static_cast<char>(_stream.u8()));
		}

		if (!result.loadFromData(buffer)) {
			throw std::runtime_error(QString("Load json [%1], position [%2]. Failed to load pixmap from stream")
				.arg(_stream.name())
				.arg(_stream.position())
				.toStdString());
		}
	}
}
