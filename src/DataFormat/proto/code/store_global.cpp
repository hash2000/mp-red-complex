#include "DataFormat/proto/code/store_global.h"

namespace Proto::Code {

	void StoreGlobal::read(DataStream &stream, ScriptContext &context) {
		const auto num = pop(stream, context).toInt();
		const auto storepos = context.svar_base + num;
		auto value = pop(stream, context);

		if (context.data.size() <= storepos) {
			throwRuntimeError(stream, QString("store pos [%1] is too long or data context is empty").arg(storepos));
		}

		context.data.at(storepos) = value;
	}

	QString StoreGlobal::name() const {
		return QString("[0x8013] [*] store_global");
	}

	QString StoreGlobal::dump() const {
		return QString("%1 = %2")
			.arg(name());
	}
} // namespace Proto::Code
