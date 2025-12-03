#pragma once
#include "Game/data_format/gam/gam.h"
#include "DataStream/data_stream.h"
#include <QString>

namespace DataFormat::Gam {

class DataReader {
public:
	enum VarMode {
		None,
		Game,
		Map,
	};

public:
  DataReader(DataStream &stream);

  void read(GlobalVariables &result);

private:
	bool readLine(QString &line);
	void parseLine(const QString &line, const VarMode mode, GlobalVariables &result);
	void throwParseException(const QString &message);

private:
  DataStream &_stream;
};

} // namespace Format::Txt
