#pragma once
#include "Launcher/resources/data_stream.h"
#include <vector>

class DataStreamBuffer : public DataStream {
public:
	DataStreamBuffer();

	virtual ~DataStreamBuffer() = default;

	size_t size() const;

	void position(size_t pos);

	void skip(size_t pos);

private:
	std::vector<char> _buffer;
};
