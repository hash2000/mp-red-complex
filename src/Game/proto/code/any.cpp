#include "Game/proto/code/any.h"

namespace Proto::Code {
	Any::Any(uint16_t opcode)
	: _opcode(opcode) {
	}

	void Any::read(DataStream &stream, ScriptContext &context) {
	}

	QString Any::name() const {
		return QString("any");
	}

	QString Any::dump() const {
		const auto lo = _opcode & 0xFF;
		const auto hi = (_opcode >> 8) & 0xFF;
		return QString("[0x%1] hex [%2 %3]")
			.arg(QString::number(_opcode, 16).toUpper().rightJustified(4, '0'))
			.arg(QString::number(lo, 16).toUpper().rightJustified(2, '0'))
    	.arg(QString::number(hi, 16).toUpper().rightJustified(2, '0'));
	}
} // namespace Proto::Code
