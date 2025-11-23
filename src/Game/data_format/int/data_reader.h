#pragma once
#include "DataStream/data_stream.h"
#include "Game/data_format/int/procedure.h"
#include <map>
#include <string>

namespace Format::Int {

	class DataReader {
	public:
		DataReader(DataStream &stream);

		void read(Programmability& result);

	private:
		using Identifiers = std::map<unsigned int, std::string>;

		void readHeader();

		void readProceduresHandles(Programmability& result);

		void readIdentifiers(Identifiers& identifiers);

		void applyProceduresNames(Programmability& result);

	private:
		DataStream &_stream;
		uint32_t _countOfProcedures = 0;
		std::vector<uint32_t> _offsets;
	};

}
