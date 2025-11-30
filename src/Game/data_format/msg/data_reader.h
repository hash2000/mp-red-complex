#pragma once
#include "DataStream/data_stream.h"
#include "Game/data_format/msg/msg.h"
#include <QString>

namespace DataFormat::Msg {

class DataReader {
public:
	enum class State {
		Outside,
		InId,
		InContext,
		InText,
	};

	enum class Expected {
		Start,
		None,
	};

public:
  DataReader(DataStream &stream);

  void read(Messages &result);

private:
  DataStream &_stream;
};

} // namespace Format::Msg
