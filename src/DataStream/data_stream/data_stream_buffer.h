#pragma once
#include "DataStream/data_stream.h"
#include <vector>
#include <span>

class DataStreamBuffer : public DataStream {
public:
	DataStreamBuffer(std::vector<char> buffer);

	DataStreamBuffer(std::span<const char> view);

	virtual ~DataStreamBuffer() = default;

	size_t size() const override;

	void position(size_t pos) override;

	void skip(size_t pos) override;

	void clear() override;

private:
	void applyBuffer();

private:
	std::vector<char> _buffer;
};
