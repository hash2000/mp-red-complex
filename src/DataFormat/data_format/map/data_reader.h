#pragma once
#include "DataStream/data_stream.h"
#include "DataFormat/proto/map.h"

namespace DataFormat::Map {
  class DataReader {
  public:
		DataReader(DataStream& stream);

		void read(Proto::Map& result);

	private:
		void readHeader(Proto::Map& result);
		void readElevations(Proto::Map& result);
		void readScripts(Proto::Map& result);
		void readObjects(Proto::Map& result);
		void readObject(Proto::MapObject& object, const Proto::Map& map);

  private:
		DataStream& _stream;
  };
}
