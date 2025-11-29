#pragma once
#include "DataStream/data_stream.h"
#include "Game/data_format/int/script.h"
#include "Game/data_format/int/procedure.h"


namespace DataFormat::Int {
class CodeReader {
public:
  CodeReader(DataStream &stream, const Programmability &prog,
             const Procedure &proc);

  void read(Script &script);

private:
  DataStream &_stream;
  const Programmability &_prog;
  const Procedure &_proc;
};
} // namespace DataFormat::Int
