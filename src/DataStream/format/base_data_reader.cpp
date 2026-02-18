#include "DataStream/format/base_data_reader.h"
#include "Resources/resources.h"

namespace Format {
	BaseDataReader::BaseDataReader(Resources* resources, const QString& container, const QString& path, const QString& name) {
		auto stream = resources->getStream(container, path);
		if (!stream.has_value()) {
			qFatal() << QString("%1: Error read %2, path %3.")
				.arg(name)
				.arg(container)
				.arg(path);
			return;
		}

		_block = stream.value()->makeBlockAsStream();
	}
}
