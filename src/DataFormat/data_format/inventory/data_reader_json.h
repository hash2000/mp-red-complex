#pragma once
#include "DataStream/data_stream.h"
#include "DataFormat/proto/inventory_item.h"

namespace DataFormat::Inv {

	class DataReaderJson {
	public:
		DataReaderJson(DataStream& stream);

		void read(Inventory& result);

	private:
		DataStream& _stream;
	};

} // namespace Format::Json
