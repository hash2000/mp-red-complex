#include "DataFormat/data_format/msg/data_reader.h"

namespace DataFormat::Msg {

DataReader::DataReader(DataStream &stream) : _stream(stream) {}

void DataReader::read(Proto::Messages &result) {
  auto state = State::Outside;
	auto expected = Expected::None;
  Proto::Message current;
	QByteArray buffer;


  while (_stream.remains() > 0) {
		const auto pos = _stream.position();
    const auto ch = static_cast<char>(_stream.u8());

		if (expected == Expected::Start) {
			if (ch != '{') {
				throw std::runtime_error(QString("%1: position [%2], expected {")
					.arg(_stream.name())
					.arg(pos)
					.toStdString());
			}

			expected = Expected::None;
			continue;
		}

    switch (state) {
    case State::Outside:
      if (ch == '{') {
				expected = Expected::None;
        state = State::InId;
        buffer.clear();
      }
      break;

    case State::InId:
      if (ch == '}') {
        current.id = buffer.toUInt();
				expected = Expected::Start;
        state = State::InContext;
        buffer.clear();
      } else {
        buffer += ch;
      }
      break;

    case State::InContext:
			if (ch == '}') {
        current.context = QString::fromUtf8(buffer);
				expected = Expected::Start;
        state = State::InText;
        buffer.clear();
      } else {
        buffer += ch;
      }
      break;

    case State::InText:
      if (ch == '}' && (buffer.endsWith('\\') == false)) {
        current.text = QString::fromUtf8(buffer);
        result.items.emplace(current.id, current);
				expected = Expected::None;
        state = State::Outside;
        buffer.clear();
      } else {
        buffer += ch;
      }
      break;
    }
  }
}
} // namespace DataFormat::Msg
