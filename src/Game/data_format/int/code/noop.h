#pragma once
#include "Game/data_format/int/instruction.h"

namespace DataFormat::Int::Code {
	class Noop : public Instructrion {
	public:
		void read(DataStream &stream, ScriptContext &context) override;
		QString dump() const override;
		QString name() const override;
	};
} // namespace DataFormat::Int::Code
