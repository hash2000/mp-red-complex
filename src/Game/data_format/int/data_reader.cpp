#include "Game/data_format/int/data_reader.h"
#include "data_reader.h"

namespace Format::Int {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
	}

	void DataReader::read(Programmability& result)
	{
		readHeader();
		readProceduresHandles(result);
		readIdentifiers(result);

	}

	void DataReader::readIdentifiers(Programmability& result) {

	}

	void DataReader::readHeader() {
		_stream.position(42);
		_countOfProcedures = _stream.uint32();
		_offsets.reserve(_countOfProcedures);
	}

  void DataReader::readProceduresHandles(Programmability &result) {
		for(uint32_t i = 0; i < _countOfProcedures; i++) {
			_offsets.push_back(_stream.uint32());

			auto proc = std::make_unique<Procedure>();
			proc->flags = _stream.uint32();
			proc->delay = _stream.uint32();
			proc->conditionOffset = _stream.uint32();
			proc->bodyOffset = _stream.uint32();
			proc->argumentsCounter = _stream.uint32();

			result.procedures.push_back(std::move(proc));
		}
  }
}
