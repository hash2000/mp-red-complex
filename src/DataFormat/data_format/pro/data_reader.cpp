#include "DataFormat/data_format/pro/data_reader.h"
#include "DataFormat/data_format/pro/critter_reader.h"
#include "DataFormat/proto/procedure.h"
#include <QRegularExpression>

namespace DataFormat::Pro {

	DataReader::DataReader(DataStream &stream)
	: _stream(stream) {
		_stream.endianness(Endianness::Little);
	}

	std::shared_ptr<Entity> DataReader::read() {
		Proto::ProtoHeader hdr;
		hdr.type = static_cast<Proto::ObjectType>(_stream.u16());
		hdr.frameId = _stream.u16();

		switch (hdr.type) {
      case Proto::ObjectType::Item:
				break;
      case Proto::ObjectType::Critter: {
				Proto::ProtoCritter item;
				CritterReader reader(_stream);
				reader.read(item);
			}
				break;
      case Proto::ObjectType::Scenery:
				break;
      case Proto::ObjectType::Wall:
				break;
      case Proto::ObjectType::Tile:
				break;
      case Proto::ObjectType::Misc:
				break;
			default:
				throw std::invalid_argument(QString("Load prototype [%1], position [%2]. Invalid type.")
					.arg(_stream.name())
					.arg(_stream.position())
					.toStdString());
    }

    return std::shared_ptr<Entity>();
	}

}
