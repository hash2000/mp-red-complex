#pragma once
#include "DataFormat/proto/instruction.h"

namespace Proto::Code {
	class PushBase : public Instructrion {
	public:
		void read(DataStream &stream, ScriptContext &context) override;
		QString dump() const override;
		QString name() const override;
	private:
		size_t dvar_base = 0;
	};
} // namespace Proto::Code
