#include "Game/data_format/sve/data_reader.h"

namespace DataFormat::Sve {

DataReader::DataReader(DataStream &stream)
: _stream(stream) {
}

void DataReader::read(Proto::ScriptEntries &result) {
  auto state = State::InOffset;
  Proto::ScriptEntry current;
  QString buffer;

  while (_stream.remains() > 0) {
		const auto pos = _stream.position();
    const auto ch = QLatin1Char(_stream.u8());

    switch (state) {
    case State::InOffset:
      if (ch == ':') {
				state = State::InText;
				current.offset = buffer.toUInt();
        buffer.clear();
      } else {
        buffer += ch;
      }
      break;

    case State::InText:
      if (isNewLine(ch)) {
        state = State::NextLine;
				current.text = buffer;
        buffer.clear();
				result.items.emplace(current.offset, current);
      } else {
        buffer += ch;
      }
      break;

    case State::NextLine:
			if (!isNewLine(ch)) {
				state = State::InOffset;
				buffer += ch;
			}
      break;
    }
  }
}

bool DataReader::isNewLine(const QChar &ch) const {
	return ch == '\r' || ch == '\n';
}
} // namespace DataFormat::Msg
