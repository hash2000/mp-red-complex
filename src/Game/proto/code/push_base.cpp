#include "Game/proto/code/push_base.h"

namespace Proto::Code {

	void PushBase::read(DataStream &stream, ScriptContext &context) {
		const auto arguments = pop(stream, context).toUInt();
		dvar_base = context.dvar_base;
		context.dvar_base = context.data.size() - arguments;
	}

	QString PushBase::name() const {
		return QString("[0x802B] [*] push_base");
	}

	QString PushBase::dump() const {
		return QString("%1 = %2")
			.arg(name())
			.arg(dvar_base);
	}
} // namespace Proto::Code
