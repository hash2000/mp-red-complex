#include "Launcher/resources/data_stream/data_stream_file.h"

DataStreamFile::DataStreamFile(std::shared_ptr<std::ifstream> stream)
	: _stream(stream) {
}

size_t DataStreamFile::size() const {
	if(_size != 0) {
		const auto pos = _stream->tellg();
		_stream->seekg(0, std::ios_base::beg);
		_size = static_cast<size_t>(_stream->tellg());
		_stream->seekg(pos, std::ios_base::beg);
	}
  return _size;
}

void DataStreamFile::position(size_t pos) {
	_stream->seekg(pos, std::ios_base::beg);
}

void DataStreamFile::skip(size_t pos) {
	const auto cur = static_cast<size_t>(_stream->tellg());
	_stream->seekg(cur + pos, std::ios_base::beg);
}
