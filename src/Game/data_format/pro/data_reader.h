#pragma once
#include "Game/proto/pro.h"
#include "DataStream/data_stream.h"
#include <QString>

namespace DataFormat::Pro {

class DataReader {
public:
  DataReader(DataStream &stream);

  void read();

private:
  DataStream &_stream;
};

} // namespace Format::Txt
