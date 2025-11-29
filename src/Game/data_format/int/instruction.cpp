#include "Game/data_format/int/instruction.h"
#include "Game/data_format/int/code/any.h"
#include "Game/data_format/int/code/noop.h"
#include "Game/data_format/int/code/push_base.h"
#include "Game/data_format/int/code/push_int.h"
#include "Game/data_format/int/code/store_global.h"
#include <sstream>

namespace DataFormat::Int {
	std::unique_ptr<Instructrion> read_instruction(uint16_t opcode) {
		switch (opcode) {
			case 0x8000: return std::make_unique<Code::Noop>(); break;
			case 0x802B: return std::make_unique<Code::PushBase>(); break;
			case 0xC001: return std::make_unique<Code::PushInt>(); break;
			case 0x8013: return std::make_unique<Code::StoreGlobal>(); break;
		}

		return std::make_unique<Code::Any>(opcode);
	}

	QVariant Instructrion::pop(DataStream &stream, ScriptContext &context) {
		const auto pos = stream.position();
		if (context.data.size() == 0) {
			throwRuntimeError(stream, "Empty stack");
		}

		auto value = context.data.back();
		context.data.pop_back();
		return value;
	}

	void Instructrion::incrementProgramSize(DataStream &stream, ScriptContext &context) {
		const auto pos = stream.position();
		const auto size = context.program_size + 4;
		const auto streamSize = stream.size();
		if (size >= streamSize) {
			throwRuntimeError(stream, QString("Expected [%1]").arg(streamSize));
		}

		context.program_size = size;
	}

	void Instructrion::throwRuntimeError(DataStream &stream, const QString &msg) {
		const auto pos = stream.position();
		const auto dmp = name().toStdString();
		std::stringstream text;
		text << dmp << ": position [" << pos << "]. " << msg.toStdString() << ".";
		throw std::runtime_error(text.str());
	}
}
