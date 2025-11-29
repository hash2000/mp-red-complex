#pragma once
#include "Game/data_format/int/instruction.h"

namespace DataFormat::Int::Code {
class Any : public Instructrion {
public:
	Any(uint16_t opcode);
	void read(DataStream &stream, ScriptContext &context) override;
	QString dump() const override;
	QString name() const override;

private:
	uint16_t _opcode;
};
} // namespace DataFormat::Int::Code
