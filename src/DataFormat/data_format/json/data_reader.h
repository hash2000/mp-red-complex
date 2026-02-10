#pragma once
#include "DataStream/data_stream.h"
#include <QJsonObject>

namespace DataFormat::Json {

	class DataReader {
	public:
		DataReader(DataStream& stream);

		void read(QJsonObject& result);

	private:
		DataStream& _stream;
	};

} // namespace Format::Json
