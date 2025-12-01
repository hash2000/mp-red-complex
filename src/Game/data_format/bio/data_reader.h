#pragma once
#include "DataStream/data_stream.h"
#include <QString>

namespace DataFormat::Bio {

class DataReader {
public:
  DataReader(DataStream &stream);

  void read(QString &result);

private:
  DataStream &_stream;
};

} // namespace Format::Txt
