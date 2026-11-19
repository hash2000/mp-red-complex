#include "Libs/DataStream/format/text/data_reader.h"
#include "Libs/DataStream/data_stream.h"
#include "Libs/Resources/resources.h"

namespace Format::Text {

	DataReader::DataReader(Resources* resources, const QString& container, const QString& path)
		: BaseDataReader(resources, container, path, "Format::Json::DataReader") {
	}

	bool DataReader::read(QString& result) {
		if (!_block) {
			return false;
		}

		QByteArray buffer;
		buffer.reserve(_block->size());
		while (_block->remains() > 0) {
			buffer.append(static_cast<char>(_block->u8()));
		}

		result = QString::fromUtf8(buffer);
		return true;
	}
}
