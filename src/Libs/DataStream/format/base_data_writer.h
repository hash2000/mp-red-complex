#pragma once
#include <QPixmap>
#include <memory>

class DataWriteStream;
class Resources;

namespace Format {
	class BaseDataWriter {
	public:
		BaseDataWriter(Resources* resources, const QString& container, const QString& path, const QString& name);
	protected:
		std::shared_ptr<DataWriteStream> _block;
	};
}
