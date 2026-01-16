#include "DataFormat/data_format/txt/data_reader.h"

namespace DataFormat::Txt {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
	}

	void DataReader::read(QString &result) {
		QByteArray buffer;
		while (_stream.remains() > 0) {
			buffer.append(static_cast<char>(_stream.u8()));
		}
		result = QString::fromUtf8(buffer);
	}
}
