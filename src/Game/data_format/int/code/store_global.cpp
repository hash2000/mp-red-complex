#include "Game/data_format/int/code/store_global.h"

namespace DataFormat::Int::Code {

	void StoreGlobal::read(DataStream &stream, ScriptContext &context) {
		const auto num = pop(stream, context).toInt();
		const auto jmpsize = context.svar_base + num;
		auto value = pop(stream, context);

		if (context.data.size() <= jmpsize) {
			throwRuntimeError(stream, QString("jump [%1] is too long or data context is empty").arg(jmpsize));
		}

		context.data.at(jmpsize) = value;
	}

	QString StoreGlobal::name() const {
		return QString("[0x8013] [*] store_global");
	}

	QString StoreGlobal::dump() const {
		return QString("%1 = %2")
			.arg(name());
	}
} // namespace DataFormat::Int::Code
