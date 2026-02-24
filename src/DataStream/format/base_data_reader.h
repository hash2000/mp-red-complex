#pragma once
#include <QPixmap>
#include <memory>

class DataStream;
class Resources;

namespace Format {
	class BaseDataReader {
	public:
		BaseDataReader(Resources* resources, const QString& container, const QString& path, const QString& name);
	protected:
		std::shared_ptr<DataStream> _block;
	};
}
