#include "DataStream/data_stream/data_stream_file.h"

DataStreamFile::DataStreamFile(std::shared_ptr<std::ifstream> stream)
	: _stream(stream) {
	if (!_stream || !_stream->is_open()) {
    throw std::invalid_argument("DataStreamFile: null or closed stream");
  }
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
	_stream->seekg(static_cast<std::streamoff>(pos), std::ios_base::beg);
}

void DataStreamFile::skip(size_t pos) {
	_stream->seekg(static_cast<std::streamoff>(pos), std::ios_base::cur);
}

void DataStreamFile::readRaw(uint8_t *dst, size_t size) {
  if (size == 0) {
    return;
  }

  if (!_stream || !_stream->good()) {
    throw std::runtime_error("DataStreamFile::readRaw: stream is invalid");
  }

  _stream->read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(size));
  const auto n = _stream->gcount();
  if (static_cast<size_t>(n) != size) {
    throw std::runtime_error("DataStreamFile::readRaw: unexpected EOF or read error");
  }
}
