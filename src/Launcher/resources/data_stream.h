#pragma once
#include "Launcher/resources/game/endianness.h"
#include <QString>
#include <type_traits>
#include <streambuf>
#include <cstdint>
#include <concepts>
#include <QByteArray>

namespace {
namespace detail {

template <typename B>
concept ByteBuffer = requires(B& b) {
    { b.resize(std::size_t{}) } -> std::same_as<void>;
    { b.data() } -> std::convertible_to<const void*>;
} && (
    std::is_same_v<std::remove_cv_t<typename B::value_type>, char> ||
    std::is_same_v<std::remove_cv_t<typename B::value_type>, unsigned char> ||
    std::is_same_v<std::remove_cv_t<typename B::value_type>, signed char> ||
    std::is_same_v<std::remove_cv_t<typename B::value_type>, std::byte>
);

} // namespace detail
} // anonymous namespace

class DataStream : public std::streambuf {
public:
	virtual ~DataStream() = default;

	virtual size_t size() const = 0;

	virtual void position(size_t pos) = 0;

	virtual size_t position() const;

	virtual void skip(size_t pos) = 0;

	virtual void readRaw(uint8_t *dst, size_t size);

	template <typename T>
	requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
	void read(T& value);

	template <typename Buffer>
	requires detail::ByteBuffer<Buffer>
	void read(Buffer& buffer);

	std::unique_ptr<DataStream> makeBlockStream();

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
		read(value);
    return *this;
  }

  uint32_t uint32() { uint32_t 	v; *this >> v; return v; }
  int32_t int32() 	{ int32_t  	v; *this >> v; return v; }
  uint16_t uint16() { uint16_t 	v; *this >> v; return v; }
  int16_t int16() 	{ int16_t 	v; *this >> v; return v; }
  uint8_t uint8() 	{ uint8_t 	v; *this >> v; return v; }
  int8_t int8() 		{ int8_t 		v; *this >> v; return v; }

private:
  template <typename T>
	static void applyEndianness(T& value, EndiannessId target);

  template <typename T>
	static T swapBytes(T value);

private:
	EndiannessId _endianness = EndiannessId::Big;
	QString _name;
	bool _isCompressed = false;
	uint32_t _decompressedSize;
	uint32_t _compressedSize;
	uint32_t _dataOffset;
};

