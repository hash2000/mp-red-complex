#pragma once
#include "Launcher/resources/data_stream.h"
#include "Launcher/resources/data_stream/data_stream_container.h"
#include <memory>
#include <list>
#include <fstream>

class DatFile : public DataStreamContainer {
public:
	void loadFromFile(const QString &path);

private:
	std::unique_ptr<DataStream> loadStream();

private:
	std::ifstream _stream;
	std::list<std::unique_ptr<DataStream>> _streams;
};
