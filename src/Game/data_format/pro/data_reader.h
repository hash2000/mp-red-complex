#pragma once
#include "Game/game/entity.h"
#include "DataStream/data_stream.h"
#include <QString>
#include <memory>

namespace DataFormat::Pro {

class DataReader {
public:
  DataReader(DataStream &stream);

  std::shared_ptr<Entity> read();

private:
  DataStream &_stream;
};

} // namespace Format::Txt
