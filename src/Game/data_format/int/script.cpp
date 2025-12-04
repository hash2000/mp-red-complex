#include "Game/data_format/int/script.h"
#include "Game/proto/instruction.h"

namespace DataFormat::Int {

	bool Script::read(DataStream &stream, Proto::ScriptContext &context, Instructions &code) {
		const auto opcode = stream.u16();
		auto instruction = Proto::read_instruction(opcode);
		if (!instruction) {
			return false;
		}

		instruction->read(stream, context);
		const auto dump = instruction->dump();

		code.push_back(std::move(instruction));
		return true;
	}

} // namespace DataFormat::Int
