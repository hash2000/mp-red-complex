#include "Launcher/resources/data_stream.h"
#include "Launcher/resources/data_stream/data_stream_buffer.h"
#include <zlib.h>

size_t DataStream::position() const {
  return gptr() - eback();
}

template <typename Buffer>
requires detail::ByteBuffer<Buffer>
inline void DataStream::read(Buffer& buffer)
{
    const std::size_t size = decompressedSize();  // ← или передавать как параметр
    buffer.resize(size);

    uint8_t* ptr = []<typename T>(T* p) -> uint8_t* {
        if constexpr (std::is_same_v<std::remove_cv_t<T>, std::byte>) {
            return reinterpret_cast<uint8_t*>(p);
        } else {
            return reinterpret_cast<uint8_t*>(p);
        }
    }(buffer.data());

    readRaw(ptr, size);
}

template <typename T>
requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
void DataStream::read(T& value) {
	readRaw(reinterpret_cast<uint8_t*>(&value), sizeof(T));
	applyEndianness(value, endianness());
}

template void DataStream::read<uint8_t>(uint8_t&);
template void DataStream::read<uint16_t>(uint16_t&);
template void DataStream::read<uint32_t>(uint32_t&);
template void DataStream::read<uint64_t>(uint64_t&);
template void DataStream::read<int32_t>(int32_t&);
template void DataStream::read<float>(float&);
template void DataStream::read<double>(double&);

void DataStream::readRaw(uint8_t* dst, size_t size) {
  if (size == 0) {
		return;
	}

  const auto n = sgetn(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(size));
  if (static_cast<size_t>(n) != size) {
    throw std::runtime_error("DataStream::readRaw: unexpected EOF");
  }
}

std::unique_ptr<DataStream> DataStream::makeBlockStream() {
	const auto decompSize = decompressedSize();
	const auto compSize = compressedSize();
	auto prevpos = position();
	position(dataOffset());

	std::vector<char> buffer;
	buffer.resize(decompSize);
	auto output = reinterpret_cast<uint8_t*>(buffer.data());

	if (compressed()) {
		std::vector<char> compressed;
		compressed.resize(compSize);
		auto zdata = reinterpret_cast<uint8_t*>(compressed.data());
		readRaw(zdata, compSize);

		z_stream zs;
		zs.total_in = compressed.size();
		zs.avail_in = compressed.size();
		zs.next_in = zdata;
		zs.total_out = decompSize;
		zs.avail_out = decompSize;
		zs.next_out = output;
		zs.zalloc = Z_NULL;
		zs.zfree = Z_NULL;
		zs.opaque = Z_NULL;

		auto ret = inflateInit(&zs);
		if (ret != Z_OK) {
			throw std::runtime_error("inflateInit failed: " + std::to_string(ret));
		}

		ret = inflate(&zs, Z_FINISH);
		const auto ok = (ret == Z_STREAM_END) &&
      (zs.total_out == decompSize);
		inflateEnd(&zs);
		if (!ok) {
      throw std::runtime_error("inflate failed or size mismatch");
		}

	} else {
		readRaw(output, decompSize);
	}

	position(prevpos);

	return std::make_unique<DataStreamBuffer>(std::move(buffer));
}

EndiannessId DataStream::endianness() const
{
    return _endianness;
}

void DataStream::endianness(EndiannessId end) {
	_endianness = end;
}

size_t DataStream::remains() const {
  return size() - position();
}

const QString DataStream::name() const {
  return _name;
}

void DataStream::name(const QString &name) {
	_name = name;
}

bool DataStream::compressed() const {
	return _isCompressed;
}

void DataStream::compressed(bool set) {
	_isCompressed = set;
}

uint32_t DataStream::decompressedSize() const {
	return _decompressedSize;
}

void DataStream::decompressedSize(uint32_t value) {
	_decompressedSize = value;
}

uint32_t DataStream::compressedSize() const {
	return _compressedSize;
}

void DataStream::compressedSize(uint32_t value) {
	_compressedSize = value;
}

uint32_t DataStream::dataOffset() const {
	return _dataOffset;
}

void DataStream::dataOffset(uint32_t value) {
	_dataOffset = value;
}

template <typename T>
T DataStream::swapBytes(T value) {
  if constexpr (sizeof(T) == 1) {
  	return value;
  } else if constexpr (sizeof(T) == 2) {
    uint16_t v = static_cast<uint16_t>(value);
    return static_cast<T>((v << 8) | (v >> 8));
  } else if constexpr (sizeof(T) == 4) {
    uint32_t v = static_cast<uint32_t>(value);
    v = ((v << 8) & 0xFF00FF00) | ((v >> 8) & 0x00FF00FF);
    return static_cast<T>((v << 16) | (v >> 16));
  } else if constexpr (sizeof(T) == 8) {
    uint64_t v = static_cast<uint64_t>(value);
    v = ((v << 8) & 0xFF00FF00FF00FF00ULL) | ((v >> 8) & 0x00FF00FF00FF00FFULL);
    v = ((v << 16) & 0xFFFF0000FFFF0000ULL) | ((v >> 16) & 0x0000FFFF0000FFFFULL);
    return static_cast<T>((v << 32) | (v >> 32));
  } else {
    static_assert(sizeof(T) == 0, "swapBytes not implemented for this size");
  }
}

template <typename T>
void DataStream::applyEndianness(T& value, EndiannessId target) {
	if (target == EndiannessId::Big) {
		swapBytes(value);
	}
}
