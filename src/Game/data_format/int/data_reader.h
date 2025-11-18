#pragma once
#include "DataStream/data_stream.h"
#include "Game/data_format/int/procedure.h"

namespace Format::Int {

	class DataReader {
	public:
		DataReader(DataStream &stream);

		void read(Programmability& result);

	private:
		void readHeader();

		void readProceduresHandles(Programmability& result);

		void readIdentifiers(Programmability& result);

	private:
		DataStream &_stream;
		uint32_t _countOfProcedures = 0;
		std::vector<uint32_t> _offsets;
	};

}
