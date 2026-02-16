#pragma once
#include <QJsonObject>
#include <memory>

class DataStream;
class Resources;

namespace Format::Json {
	class DataReader {
	public:
		DataReader(Resources* resources, const QString& container, const QString& path);
		bool read(QJsonObject& result);
	private:
		std::shared_ptr<DataStream> _block;
	};
}
