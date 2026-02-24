#pragma once
#include "DataStream/format/base_data_reader.h"
#include <QJsonObject>
#include <memory>

class DataStream;
class Resources;

namespace Format::Json {
	class DataReader : public BaseDataReader {
	public:
		DataReader(Resources* resources, const QString& container, const QString& path);
		bool read(QJsonObject& result);
	};
}
