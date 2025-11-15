#pragma once
#include "Resources/resources/data_stream.h"
#include "Resources/resources/data_stream/data_stream_container.h"
#include <memory>
#include <fstream>

class DatFile : public DataStreamContainer {
public:
	void loadFromFile(const QString &path);

private:
	std::unique_ptr<DataStream> loadStream(std::shared_ptr<std::ifstream> &stream);
};
