#pragma once
#include "Libs/DataStream/data_stream.h"
#include "Libs/DataFormat/proto/script_context.h"
#include "Libs/DataFormat/proto/instruction.h"

namespace DataFormat::Int {
	class Script {
	public:
		using Instructions = std::list<std::unique_ptr<Proto::Instructrion>>;

	public:
		bool read(DataStream &stream, Proto::ScriptContext &context, Instructions &code);
	};
} // namespace DataFormat::Int
