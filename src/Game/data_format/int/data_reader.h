#pragma once
#include "DataStream/data_stream.h"
#include "Game/data_format/int/procedure.h"
#include <map>
#include <string>

namespace DataFormat::Int {

	class DataReader {
	public:
		DataReader(DataStream &stream);

		void read(Programmability &result);

	private:
		using Identifiers = std::map<unsigned int, std::string>;

		void readHeader(Header &header);
		void readProceduresHandles(Programmability &result);
		void readIdentifiers(Identifiers &identifiers, const std::string &trait);
		void applyProceduresNames(Programmability &result);

	private:
		DataStream &_stream;
		std::vector<uint32_t> _offsets;
	};

}
