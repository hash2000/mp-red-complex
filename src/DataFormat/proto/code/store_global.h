#pragma once
#include "DataFormat/proto/instruction.h"

namespace Proto::Code {
	class StoreGlobal : public Instructrion {
	public:
		void read(DataStream &stream, ScriptContext &context) override;
		QString dump() const override;
		QString name() const override;
	};
} // namespace Proto::Code
