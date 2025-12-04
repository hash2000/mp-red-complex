#pragma once
#include "Game/proto/gcd.h"
#include "Game/proto/gcd_traits.h"
#include "DataStream/data_stream.h"
#include <QString>
#include <QDebug>

namespace DataFormat::Gcd {

class DataReader {
public:
  DataReader(DataStream &stream);

  void read(Proto::Character &result);

private:
	template <class Enum>
	void readValues(std::vector<uint32_t> &values, const std::vector<uint32_t> &stats, const QString &description) {
		constexpr auto first = static_cast<uint32_t>(Proto::EnumTraits<Enum>::first);
    constexpr auto last  = static_cast<uint32_t>(Proto::EnumTraits<Enum>::last);
		values.resize(last + 1);
		for (uint32_t i = first; i <= last; i++) {
			const auto value = _stream.u32();
			const auto state = static_cast<Enum>(i);
			if (!Proto::Validator<Enum>::validate(state, value, stats)) {
				throwReadError(i, description);
			}

			values[i] = value;
		}
	}

	void readAge(const Proto::Character &result, uint32_t &age);
	void readGender(const Proto::Character &result, Proto::Gender &gender);
	void readName(QString &name);

	void throwReadError(uint32_t stat, const QString &description);

private:
  DataStream &_stream;
};

} // namespace Format::Txt
