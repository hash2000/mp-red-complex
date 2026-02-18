#pragma once
#include "DataStream/format/base_data_reader.h"
#include <QPixmap>
#include <memory>

class DataStream;
class Resources;

namespace Format::Pixmap {
	class DataReader : public BaseDataReader {
	public:
		DataReader(Resources* resources, const QString& container, const QString& path);
		bool read(QPixmap& result);
	};
}
