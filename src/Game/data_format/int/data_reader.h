#pragma once
#include "DataStream/data_stream.h"
#include "Game/proto/procedure.h"
#include <map>
#include <string>

namespace DataFormat::Int {

	class DataReader {
	public:
		DataReader(DataStream &stream);

		void read(Proto::Programmability &result);

	private:
		using Identifiers = std::map<unsigned int, std::string>;

		void readHeader(Proto::Header &header);
		void readProceduresHandles(Proto::Programmability &result);
		void readIdentifiers(Identifiers &identifiers, const std::string &trait);
		void applyProceduresNames(Proto::Programmability &result);

	private:
		DataStream &_stream;
		std::vector<uint32_t> _offsets;
	};

}
