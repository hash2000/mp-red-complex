#pragma once
#include "DataStream/data_stream.h"
#include "Game/proto/procedure.h"
#include "Game/proto/script_context.h"
#include <QVariant>
#include <vector>
#include <memory>

namespace Proto {
	class Instructrion {
	public:
		virtual ~Instructrion() = default;

		virtual void read(DataStream &stream, ScriptContext &context) = 0;
		virtual QString dump() const = 0;
		virtual QString name() const = 0;

	protected:
		QVariant pop(DataStream &stream, ScriptContext &context);
		void throwRuntimeError(DataStream &stream, const QString &msg);

	protected:
		std::vector<QVariant> _args;
		OpCode _opcode = OpCode::Noop;
	};

	std::unique_ptr<Instructrion> read_instruction(uint16_t opcode);

} // namespace Proto
