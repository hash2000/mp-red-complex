#pragma once
#include "Launcher/resources/game/endianness.h"
#include <QString>
#include <type_traits>
#include <streambuf>
#include <cstdint>

class DataStream : public std::streambuf {
public:
	virtual ~DataStream() = default;

	virtual size_t size() const = 0;

	virtual std::streambuf::int_type underflow();

	virtual void position(size_t pos) = 0;

	virtual size_t position() const;

	virtual void skip(size_t pos) = 0;

	virtual void read(uint8_t* destination, size_t size);

	EndiannessId endianness() const;

	void endianness(EndiannessId end);

	size_t remains() const;

	const QString name() const;

	void name(const QString &name);

	bool compressed() const;

	void compressed(bool set);

	uint32_t decompressedSize() const;

	void decompressedSize(uint32_t value);

	uint32_t compressedSize() const;

	void compressedSize(uint32_t value);

	uint32_t dataOffset() const;

	void dataOffset(uint32_t value);

public:
	template<class T> std::enable_if_t<std::is_integral_v<T>, DataStream&> operator>>(T& value) {
    static_assert(std::is_trivially_copyable_v<T>, "Only trivially copyable types allowed");
    uint8_t* buff = reinterpret_cast<uint8_t*>(&value);
    read(buff, sizeof(value));
    return *this;
  }

  uint32_t uint32() { uint32_t 	v; *this >> v; return v; }
  int32_t int32() 	{ int32_t  	v; *this >> v; return v; }
  uint16_t uint16() { uint16_t 	v; *this >> v; return v; }
  int16_t int16() 	{ int16_t 	v; *this >> v; return v; }
  uint8_t uint8() 	{ uint8_t 	v; *this >> v; return v; }
  int8_t int8() 		{ int8_t 		v; *this >> v; return v; }

private:
	EndiannessId _endianness = EndiannessId::Big;
	QString _name;
	bool _isCompressed = false;
	uint32_t _decompressedSize;
	uint32_t _compressedSize;
	uint32_t _dataOffset;
};
