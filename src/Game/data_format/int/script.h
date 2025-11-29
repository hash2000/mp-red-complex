#pragma once
#include "DataStream/data_stream.h"
#include "Game/data_format/int/script_context.h"
#include "Game/data_format/int/instruction.h"

namespace DataFormat::Int {
	class Script {
	public:
		using Instructions = std::list<std::unique_ptr<Instructrion>>;

	public:
		bool read(DataStream &stream, ScriptContext &context, Instructions &code);
	};
} // namespace DataFormat::Int
