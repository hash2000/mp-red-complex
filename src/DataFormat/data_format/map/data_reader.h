#pragma once
#include "DataStream/data_stream.h"

namespace DataFormat::Map {
	class DataReader {
	public:
		DataReader(DataStream &stream);

	private:
		DataStream &_stream;
	};
}
