#pragma once
#include "DataFormat/proto/pro.h"
#include "DataStream/data_stream.h"

namespace DataFormat::Pro {

class CritterReader {
public:
  CritterReader(DataStream &stream);

  void read(Proto::ProtoCritter &result);

private:
  DataStream &_stream;
};

}
