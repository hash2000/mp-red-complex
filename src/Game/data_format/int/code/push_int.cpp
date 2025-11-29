#include "Game/data_format/int/code/push_int.h"

namespace DataFormat::Int::Code {

	void PushInt::read(DataStream &stream, ScriptContext &context) {
		_value = stream.i32();
		context.data.push_back(_value);

		incrementProgramSize(stream, context);
	}

	QString PushInt::name() const {
		return QString("[0xC001] [*] push_int");
	}

	QString PushInt::dump() const {
		return QString("%1 = %2")
			.arg(name())
			.arg(_value);
	}
} // namespace DataFormat::Int::Code
