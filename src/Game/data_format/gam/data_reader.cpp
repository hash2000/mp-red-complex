#include "Game/data_format/gam/data_reader.h"
#include <QRegularExpression>

namespace DataFormat::Gam {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
	}

	void DataReader::read(Proto::GlobalVariables &result) {

		auto mode = VarMode::None;

		QString line;
		while (readLine(line)) {

			QString trimmed = line.trimmed();
			line.clear();

			if (trimmed.isEmpty() || trimmed.startsWith("//")) {
				continue;
			}

			if (trimmed == "MAP_GLOBAL_VARS:")	{
				mode = VarMode::Map;
				continue;
			}

			if (trimmed == "GAME_GLOBAL_VARS:")	{
				mode = VarMode::Game;
				continue;
			}

			parseLine(trimmed, mode, result);
		}
	}

	void DataReader::parseLine(const QString &line, const VarMode mode, Proto::GlobalVariables &result) {
		static const QRegularExpression rx(
      R"(^\s*(\w+)\s*:=\s*(-?\d+)\s*;\s*(?:\/\/\s*(.*))?$)"
		);

		if (mode == VarMode::None) {
			throwParseException("Undefined variables mode");
		}

		const auto match = rx.match(line);
		if (match.hasMatch()) {
			auto item = std::make_shared<Proto::GlobalVar>();
			bool ok;

			item->name = match.captured(1);
			item->value = match.captured(2).toInt(&ok);
			if (!ok) {
				throwParseException("value is not int type");
			}

			item->comment = match.captured(3);

			int autoIndex = 0;

			if (mode == VarMode::Game) {
				autoIndex = result.gameVars.size();
			} else if (mode == VarMode::Map){
				autoIndex = result.mapVars.size();
			}

			item->index = autoIndex;

			if (mode == VarMode::Game) {
				result.gameVars.emplace(item->name, item);
			} else if (mode == VarMode::Map){
				result.mapVars.emplace(item->name, item);
			}
		}
	}

	void DataReader::throwParseException(const QString &message) {
		const auto pos = _stream.position();
		throw std::runtime_error(QString("Parse script [%1] error, position [%2]. %3")
			.arg(_stream.name())
			.arg(pos)
			.arg(message)
			.toStdString());
	}

	bool DataReader::readLine(QString &line) {
		while (_stream.remains() > 0) {
			const auto ch = QLatin1Char(_stream.u8());

			if (ch == '\n') {
				return true;
			}

			line += ch;
		}

		return false;
	}
}
