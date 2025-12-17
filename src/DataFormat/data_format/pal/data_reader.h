#pragma once
#include "DataFormat/proto/pal.h"
#include "DataStream/data_stream.h"
#include <QString>

namespace DataFormat::Pal {

class DataReader {
public:
  DataReader(DataStream &stream);

  void read(Proto::Pallete &result);

private:
  DataStream &_stream;
};

} // namespace Format::Txt
