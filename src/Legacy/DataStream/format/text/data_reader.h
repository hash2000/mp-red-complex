#pragma once
#include "Libs/DataStream/format/base_data_reader.h"

class DataStream;
class Resources;

namespace Format::Text {
	class DataReader : public BaseDataReader {
	public:
		DataReader(Resources* resources, const QString& container, const QString& path);
		bool read(QString& result);
	};
}
