#pragma once
#include <QPixmap>
#include <memory>

class DataStream;
class Resources;

namespace Format::Pixmap {
	class DataReader {
	public:
		DataReader(Resources* resources, const QString& container, const QString& path);
		bool read(QPixmap& result);
	private:
		std::shared_ptr<DataStream> _block;
	};
}
