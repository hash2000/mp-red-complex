#pragma once
#include "DataStream/data_stream.h"
#include "Game/proto/frm.h"
#include <QString>

namespace DataFormat::Frm {

class DataReader {
public:
  DataReader(DataStream &stream);

  void read(Proto::Animation &result, const QString &suffix);

private:
	void readDirectionsOffsets(std::vector<Proto::Direction> &directions);
	void readFrames(Proto::Animation &result);

private:
  DataStream &_stream;
	uint32_t _bodyPos = 0;
};

} // namespace Format::Txt
