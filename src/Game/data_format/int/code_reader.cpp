#include "Game/data_format/int/code_reader.h"

namespace DataFormat::Int {
	CodeReader::CodeReader(DataStream &stream, const Proto::Programmability &prog, const Proto::Procedure &proc)
	: _stream(stream)
	, _prog(prog)
	, _proc(proc)
	{
	}

	void CodeReader::read(Script &script) {
		using namespace Proto;

		Script::Instructions code;
		ScriptContext context;
		context.data.push_back(0);
		context.returns.push_back(0);

		_stream.position(_proc.bodyOffset);
		try {
			while (_stream.remains() > 0) {
				if (!script.read(_stream, context, code)) {
					return;
				}
			}
		}
		catch (std::exception &ex) {
			qDebug() << "Processing script [" << _proc.name << "] error: " << ex.what();
		}
	}

}
