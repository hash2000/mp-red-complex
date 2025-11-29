#pragma once
#include "Game/data_format/int/instruction.h"

namespace DataFormat::Int::Code {
	class PushBase : public Instructrion {
	public:
		void read(DataStream &stream, ScriptContext &context) override;
		QString dump() const override;
		QString name() const override;
	private:
		size_t dvar_base = 0;
	};
} // namespace DataFormat::Int::Code
