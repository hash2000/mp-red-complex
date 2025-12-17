#pragma once
#include "DataStream/data_stream.h"
#include "DataFormat/proto/script_context.h"
#include "DataFormat/proto/instruction.h"

namespace DataFormat::Int {
	class Script {
	public:
		using Instructions = std::list<std::unique_ptr<Proto::Instructrion>>;

	public:
		bool read(DataStream &stream, Proto::ScriptContext &context, Instructions &code);
	};
} // namespace DataFormat::Int
