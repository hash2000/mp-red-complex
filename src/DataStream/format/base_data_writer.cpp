#include "DataStream/format/base_data_writer.h"
#include "Resources/resources.h"

namespace Format {
	BaseDataWriter::BaseDataWriter(Resources* resources, const QString& container, const QString& path, const QString& name) {
		auto stream = resources->getWriteStream(container, path);
		if (!stream.has_value()) {
			qWarning() << QString("%1: Error read %2, path %3.")
				.arg(name)
				.arg(container)
				.arg(path);
			return;
		}

		_block = stream.value();
	}
}
