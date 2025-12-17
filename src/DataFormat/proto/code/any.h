#pragma once
#include "DataFormat/proto/instruction.h"

namespace Proto::Code {
class Any : public Instructrion {
public:
	Any(uint16_t opcode);
	void read(DataStream &stream, ScriptContext &context) override;
	QString dump() const override;
	QString name() const override;

private:
	uint16_t _opcode;
};
} // namespace Proto::Code
