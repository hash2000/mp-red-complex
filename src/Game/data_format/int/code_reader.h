#pragma once
#include "DataStream/data_stream.h"
#include "Game/data_format/int/script.h"
#include "Game/proto/procedure.h"


namespace DataFormat::Int {
class CodeReader {
public:
  CodeReader(DataStream &stream, const Proto::Programmability &prog,
             const Proto::Procedure &proc);

  void read(Script &script);

private:
  DataStream &_stream;
  const Proto::Programmability &_prog;
  const Proto::Procedure &_proc;
};
} // namespace DataFormat::Int
