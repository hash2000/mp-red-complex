#pragma once
#include "Game/data_format/int/instruction.h"

namespace DataFormat::Int::Code {
	class PushInt : public Instructrion {
	public:
		void read(DataStream &stream, ScriptContext &context) override;
		QString dump() const override;
		QString name() const override;
	private:
		uint32_t _value = 0;
	};
} // namespace DataFormat::Int::Code
