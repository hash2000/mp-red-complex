#pragma once
#include "DataStream/data_stream.h"
#include "Game/data_format/sve/sve.h"
#include <QString>

namespace DataFormat::Sve {

class DataReader {
public:
	enum class State {
		InOffset,
		InText,
		NextLine,
	};

public:
  DataReader(DataStream &stream);

  void read(ScriptEntries &result);

private:
	bool isNewLine(const QChar &ch) const;

private:
  DataStream &_stream;
};

} // namespace Format::Msg
