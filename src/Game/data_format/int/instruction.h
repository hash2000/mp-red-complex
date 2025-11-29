#pragma once
#include "DataStream/data_stream.h"
#include "Game/data_format/int/procedure.h"
#include "Game/data_format/int/script_context.h"
#include <QVariant>
#include <vector>
#include <memory>

namespace DataFormat::Int {
	class Instructrion {
	public:
		virtual ~Instructrion() = default;

		virtual void read(DataStream &stream, ScriptContext &context) = 0;
		virtual QString dump() const = 0;
		virtual QString name() const = 0;

	protected:
		void incrementProgramSize(DataStream &stream, ScriptContext &context);
		QVariant pop(DataStream &stream, ScriptContext &context);
		void throwRuntimeError(DataStream &stream, const QString &msg);

	protected:
		std::vector<QVariant> _args;
		OpCode _opcode = OpCode::Noop;
	};

	std::unique_ptr<Instructrion> read_instruction(uint16_t opcode);
}
