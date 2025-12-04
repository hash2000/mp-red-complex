#pragma once
#include "Game/proto/instruction.h"

namespace Proto::Code {
	class Noop : public Instructrion {
	public:
		void read(DataStream &stream, ScriptContext &context) override;
		QString dump() const override;
		QString name() const override;
	};
} // namespace Proto::Code
