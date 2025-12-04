#include "Game/proto/code/noop.h"

namespace Proto::Code {

	void Noop::read(DataStream &stream, ScriptContext &context) {
	}

	QString Noop::name() const {
		return QString("[0x8000] noop");
	}

	QString Noop::dump() const {
		return name();
	}
} // namespace Proto::Code
