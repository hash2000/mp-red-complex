#pragma once
#include "DataStream/data_stream.h"
#include "DataStream/data_stream/data_stream_container.h"
#include <QDir>

class RawDirectory : public DataStreamContainer {
public:
	void loadFromPath(const QString &path);
	ContainerType type() const override;

private:
	QDir _path;
};
