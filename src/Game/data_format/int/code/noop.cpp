#include "Game/data_format/int/code/noop.h"

namespace DataFormat::Int::Code {

	void Noop::read(DataStream &stream, ScriptContext &context) {
	}

	QString Noop::name() const {
		return QString("[0x8000] noop");
	}

	QString Noop::dump() const {
		return name();
	}
} // namespace DataFormat::Int::Code
