#pragma once
#include "Resources/resources/data_stream.h"
#include <vector>
#include <span>

class DataStreamBuffer : public DataStream {
public:
	DataStreamBuffer(std::vector<char> buffer);

	DataStreamBuffer(std::span<const char> view);

	virtual ~DataStreamBuffer() = default;

	size_t size() const;

	void position(size_t pos);

	void skip(size_t pos);

private:
	void applyBuffer();

private:
	std::vector<char> _buffer;
};
