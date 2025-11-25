#include "Game/data_format/txt/data_reader.h"

namespace DataFormat::Txt {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
	}

	void DataReader::read(QString &result) {
		while (_stream.remains() > 0) {
			result.append(QLatin1Char(_stream.u8()));
		}
	}
}
