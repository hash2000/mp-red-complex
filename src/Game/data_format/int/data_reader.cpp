#include "Game/data_format/int/data_reader.h"

namespace Format::Int {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
	}

	void DataReader::read(Programmability& result)
	{
		readHeader();
		readProceduresHandles(result);
		readIdentifiers(result.identifiers);
		applyProceduresNames(result);
		readIdentifiers(result.strings);
	}

	void DataReader::readIdentifiers(Identifiers& identifiers) {
		const auto tableSize = _stream.u32();
		uint32_t j = 0;
		while (j < tableSize) {
			const auto nameSize = _stream.u16();
			j += 2;

			const auto nameOffset = j + 4;
			std::string name;

			for (uint32_t i = 0; i < nameSize; i++, j++) {
				const auto ch = _stream.i8();
				if (ch != 0) {
					name.push_back(ch);
				}
			}

			identifiers.emplace(nameOffset, name);
		}

		const auto pos = _stream.position();
		const auto eol = _stream.u32();
		if (eol != 0xffffffff) {
			throw std::runtime_error("Format::Int::DataReader::readIdentifiers: EOF not found");
		}
	}

	void DataReader::applyProceduresNames(Programmability& result) {
		for (uint32_t i = 0, size = _offsets.size(); i < size ;i++) {
			const auto offset = _offsets.at(i);
			const auto &name = result.identifiers.at(offset);
			result.procedures.at(i)->name = name;
		}
	}

	void DataReader::readHeader() {
		_stream.position(42);
		_countOfProcedures = _stream.u32();
		_offsets.reserve(_countOfProcedures);
	}

  void DataReader::readProceduresHandles(Programmability &result) {
		for(uint32_t i = 0; i < _countOfProcedures; i++) {
			_offsets.push_back(_stream.u32());

			auto proc = std::make_unique<Procedure>();
			proc->flags = _stream.u32();
			proc->delay = _stream.u32();
			proc->conditionOffset = _stream.u32();
			proc->bodyOffset = _stream.u32();
			proc->argumentsCounter = _stream.u32();
			proc->isTimed = has_flag(proc->flags, ProcedureIType::Timed);
			proc->isConditional = has_flag(proc->flags, ProcedureIType::Conditional);
			proc->isCritical = has_flag(proc->flags, ProcedureIType::Critical);
			proc->isExported = has_flag(proc->flags, ProcedureIType::Exported);
			proc->isImported = has_flag(proc->flags, ProcedureIType::Imported);
			proc->isInline = has_flag(proc->flags, ProcedureIType::Inline);
			proc->isPure = has_flag(proc->flags, ProcedureIType::Pure);

			result.procedures.push_back(std::move(proc));
		}
  }
}
