#pragma once
#include "Libs/DataFormat/proto/gam.h"
#include "Libs/DataStream/data_stream.h"
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

  void read(Proto::GlobalVariables &result);

private:
	bool readLine(QString &line);
	void parseLine(const QString &line, const VarMode mode, Proto::GlobalVariables &result);
	void throwParseException(const QString &message);

private:
  DataStream &_stream;
};

} // namespace Format::Txt
