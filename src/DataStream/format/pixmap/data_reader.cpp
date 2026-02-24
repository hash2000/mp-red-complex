#include "DataStream/format/pixmap/data_reader.h"
#include "DataStream/data_stream.h"
#include "Resources/resources.h"

namespace Format::Pixmap {

	DataReader::DataReader(Resources* resources, const QString& container, const QString& path)
	: BaseDataReader(resources, container, path, "Format::Pixmap::DataReader") {
	}

	bool DataReader::read(QPixmap& result) {
		if (!_block) {
			return false;
		}

		QByteArray buffer;
		buffer.reserve(_block->size());
		while (_block->remains() > 0) {
			buffer.append(static_cast<char>(_block->u8()));
		}

		if (!result.loadFromData(buffer)) {
			qFatal() << QString("Load Pixmap [%1], position [%2]. Failed to load pixmap from stream")
				.arg(_block->name())
				.arg(_block->position());
			return false;
		}

		return true;
	}
}
