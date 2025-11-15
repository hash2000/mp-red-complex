#pragma once
#include "Launcher/resources/data_stream.h"
#include <fstream>
#include <memory>

class DataStreamFile : public DataStream {
public:
	DataStreamFile(std::shared_ptr<std::ifstream> stream);

	virtual ~DataStreamFile() = default;

	size_t size() const override;

	void position(size_t pos) override;

	void skip(size_t pos) override;

	void readRaw(uint8_t *dst, size_t size) override;

private:
	std::shared_ptr<std::ifstream> _stream;
	mutable size_t _size = 0;
};

