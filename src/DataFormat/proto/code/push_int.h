#pragma once
#include "DataFormat/proto/instruction.h"

namespace Proto::Code {
	class PushInt : public Instructrion {
	public:
		void read(DataStream &stream, ScriptContext &context) override;
		QString dump() const override;
		QString name() const override;
	private:
		uint32_t _value = 0;
	};
} // namespace Proto::Code
