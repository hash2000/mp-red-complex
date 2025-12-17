#include "Game/data_format/pal/data_reader.h"

namespace DataFormat::Pal {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
	}

	void DataReader::read(Proto::Pallete &result) {
		result.items.resize(256);
		for (int i = 0; i < 256; ++i) {
				uchar r = _stream.u8();
				uchar g = _stream.u8();
				uchar b = _stream.u8();
				result.items[i] = qRgb(r, g, b);
		}
	}
}
