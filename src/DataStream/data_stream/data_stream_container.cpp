#include "DataStream/data_stream/data_stream_container.h"

const QString DataStreamContainer::name() const {
  return _name;
}

void DataStreamContainer::name(const QString &name) {
	_name = name;
}

