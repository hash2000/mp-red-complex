#pragma once
#include <concepts>

enum class Endianness : uint8_t {
	Big = 0,
	Little,
};

enum class ContainerType : uint8_t {
	Undefined,
	Repository_v1,
	Directory,
};

#if __cpp_lib_byteswap >= 202110L
#	include <bit>
#else
namespace detail {
	template <typename T> constexpr T byteswap_fallback(T value) noexcept {
		if constexpr (sizeof(T) == 1) {
			return value;
		}
		else if constexpr (sizeof(T) == 2) {
			return static_cast<T>(((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8));
		}
		else if constexpr (sizeof(T) == 4) {
			return static_cast<T>(
				((value & 0xFF000000u) >> 24) | ((value & 0x00FF0000u) >> 8) |
				((value & 0x0000FF00u) << 8) | ((value & 0x000000FFu) << 24));
		}
		else if constexpr (sizeof(T) == 8) {
			return static_cast<T>(
				((value & 0xFF00000000000000ull) >> 56) |
				((value & 0x00FF000000000000ull) >> 40) |
				((value & 0x0000FF0000000000ull) >> 24) |
				((value & 0x000000FF00000000ull) >> 8) |
				((value & 0x00000000FF000000ull) << 8) |
				((value & 0x0000000000FF0000ull) << 24) |
				((value & 0x000000000000FF00ull) << 40) |
				((value & 0x00000000000000FFull) << 56));
		}
		else {
			static_assert(sizeof(T) == 0, "byteswap not implemented for this size");
		}
	}
} // namespace detail
#endif

namespace detail {

	template <typename B>
	concept ByteBuffer =
		requires(B & b) {
			{ b.resize(std::size_t{}) } -> std::same_as<void>;
			{ b.data() };
			{ b.size() } -> std::convertible_to<std::size_t>;
	} &&
		(std::same_as<std::remove_cv_t<typename B::value_type>, char> ||
			std::same_as<std::remove_cv_t<typename B::value_type>, unsigned char> ||
			std::same_as<std::remove_cv_t<typename B::value_type>, signed char> ||
			std::same_as<std::remove_cv_t<typename B::value_type>, std::byte>);

	template <typename B>
	concept ReadOnlyByteBuffer = requires(const B & b) {
		{ b.data() };
		{ b.size() } -> std::convertible_to<std::size_t>;
	} &&
		(std::same_as<std::remove_cv_t<typename B::value_type>, char> ||
			std::same_as<std::remove_cv_t<typename B::value_type>, unsigned char> ||
			std::same_as<std::remove_cv_t<typename B::value_type>, signed char> ||
			std::same_as<std::remove_cv_t<typename B::value_type>, std::byte>);

} // namespace detail
