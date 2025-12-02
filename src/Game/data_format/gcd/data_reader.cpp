#include "Game/data_format/gcd/data_reader.h"

namespace DataFormat::Gcd {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
	}

	void DataReader::read(Character &result) {
		_stream.skip(4);
		readValues<Stat>(result.stat, result.stat, "Stats");
		readValues<StatSecondary>(result.statsSecondary, result.stat, "Stats secondary");
		readValues<Damage>(result.damage, result.stat, "Damage treshold");
		readValues<DamageResistance>(result.damageResistance, result.stat, "Damage resistance");
		readAge(result, result.age);
		readGender(result, result.gender);
		readValues<Stat>(result.statsBonus, result.stat, "Stats bonuses");
		readValues<StatSecondary>(result.statsSecondaryBonus, result.stat, "Stats secondary bonuses");
		readValues<Damage>(result.damageBonus, result.stat, "Damage bonuses");
		readValues<DamageResistance>(result.damageResistanceBonus, result.stat, "Damage resistance bonuses");
		readAge(result, result.ageBonus);
		readGender(result, result.genderBonus);

		readValues<Skill>(result.skill, result.stat, "Skills");
		_stream.skip(16);

		readName(result.name);
		readValues<Tagged>(result.tagged, result.stat, "Tagged");
	}

	void DataReader::readName(QString &name) {
		for (uint32_t i = 0; i < 32; i++) {
			const auto value = _stream.u8();
			if (value != 0) {
				name += QLatin1Char(value);
			}
		}
	}

	void DataReader::readGender(const Character &result, Gender &gender) {
		const auto value = _stream.u32();
		gender = to_state<Gender>(value);
	}

	void DataReader::readAge(const Character &result, uint32_t &age) {
		age = _stream.u32();
		if (!Validator<Age>::validate(Age::Current, age, result.stat)) {
			throwReadError(to_u32(Age::Current), "Age");
		}
	}

	void DataReader::throwReadError(uint32_t stat, const QString &description) {
		const auto pos = _stream.position();
		throw std::runtime_error(
			QString("%1 %2 [%3], validation error")
				.arg(_stream.name())
				.arg(description)
				.arg(stat)
				.toStdString());
	}

}
