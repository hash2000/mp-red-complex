#pragma once
#include "Libs/DataStream/format/base_data_writer.h"
#include <QJsonObject>

class DataWriteStream;
class Resources;

namespace Format::Json {
	class DataWriter : public BaseDataWriter {
	public:
		DataWriter(Resources* resources, const QString& container, const QString& path);
		bool write(const QJsonObject& buffer);
	};
}
