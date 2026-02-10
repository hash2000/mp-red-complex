#pragma once
#include "DataStream/data_stream.h"
#include <QPixmap>

namespace DataFormat::Pixmap {

class DataReader {
public:
  DataReader(DataStream &stream);

  void read(QPixmap&result);

private:
  DataStream &_stream;
};

} // namespace Format::Png
