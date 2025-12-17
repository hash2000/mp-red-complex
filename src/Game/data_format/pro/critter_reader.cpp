#include "Game/data_format/pro/critter_reader.h"
#include <QRegularExpression>

namespace DataFormat::Pro {

	CritterReader::CritterReader(DataStream &stream)
	: _stream(stream) {
		_stream.endianness(Endianness::Big);
	}

  void CritterReader::read(Proto::ProtoCritter &result) {
		_stream >> result.scriptId;

		for (int i = 0; i < 10; i++) {
			_stream >> result.inventory[i];
		}

		_stream >> result.hp >> result.max_hp;

		for (int i = 0; i < 45; i++) {
			_stream >> result.stats[i];
		}

		result.team_num = _stream.u8();
		result.ai_packet = _stream.u8();

		for (int i = 0; i < 6; i++) {
			result.damage_resist[i] = _stream.u8();
		}

		for (int i = 0; i < 6; i++) {
			result.damage_thresh[i] = _stream.u8();
		}

		result.perk = _stream.u8();
		result.gender = _stream.u8();
		_stream.skip(2);

		_stream >> result.look_at >> result.talk_at >> result.critter_flags;

		result.who_hit_me = _stream.u8();
		_stream.skip(3);

		_stream >> result.reaction >> result.kill_exp >> result.body_type
          	>> result.head_fid >> result.ai_tactics;

		result.radiation_level = _stream.u8();
		result.poison_level = _stream.u8();
		_stream.skip(2);

		_stream >> result.script_overrides >> result.combat_difficulty;
		_stream.skip(328);

		const auto remains = _stream.remains();
		qDebug() << "Load critter, remains bytes " << remains;
	}
}
