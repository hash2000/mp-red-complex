#include "DataFormat/data_format/map/data_reader.h"

namespace DataFormat::Map {
  DataReader::DataReader(DataStream& stream)
	: _stream(stream) {
  }

  void DataReader::read(Proto::Map& result) {
		readHeader(result);
		readElevations(result);
		readScripts(result);
		readObjects(result);
  }

	void DataReader::readHeader(Proto::Map& result) {
		result.version = _stream.u32();

		result.name.clear();

		for (int i = 0; i < 16; i++) {
			const auto ch = _stream.u8();
			if (ch == 0) {
				continue;
			}
			result.name.push_back(QLatin1Char(ch));
		}

		result.name = result.name.toLower();
		result.position = _stream.i32();
		result.elevation = _stream.i32();
		result.orientation = _stream.i32();
		result.lvarsSize = _stream.i32();
		result.scriptId = _stream.i32();
		result.flags = _stream.i32();
		result.darkness = _stream.i32();
		result.mvarsSize = _stream.i32();
		result.id = _stream.i32();
		result.epochTime = _stream.i32();

		_stream.skip(4 * 44);

		result.mvars.resize(result.mvarsSize);
		for (int i = 0; i < result.mvarsSize; i++) {
			result.mvars[i] = _stream.i32();
		}

		result.lvars.resize(result.lvarsSize);
		for (int i = 0; i < result.lvarsSize; i++) {
			result.lvars[i] = _stream.i32();
		}
	}

	void DataReader::readElevations(Proto::Map& result) {

		uint32_t elevations = 0;
		if ((result.flags & 2) == 0) {
			elevations++;
		}

		if ((result.flags & 4) == 0) {
			elevations++;
		}

		if ((result.flags & 8) == 0) {
			elevations++;
		}

		result.elevations.resize(elevations);

		for (int i = 0; i < elevations; i++) {
			for (int j = 0; j < 10000; j++) {
				result.elevations[i].roofs.push_back(_stream.u16());
				result.elevations[i].floors.push_back(_stream.u16());
			}
		}
	}

	void DataReader::readScripts(Proto::Map& result) {
		for (int i = 0; i < 5; i++) {
			const auto count = _stream.u32();
			if (count == 0) {
				continue;
			}

			auto loop = count;
			auto loopMod = loop % 16;
			if (loopMod > 0) {
				loop += 16 - loopMod;
			}

			uint32_t check = 0;
			for (int j = 0; j < loop; j++) {
				Proto::MapScript script;
				script.pid = _stream.i32();
				script.nextScriptId = _stream.i32();
				const auto type = (script.pid & 0xFF000000) >> 24;
				switch (type) {
				case 0:
					script.type = Proto::MapScriptType::System;
					break;
				case 1:
					script.type = Proto::MapScriptType::Spatial;
					script.spatialTile = _stream.i32();
					script.spatialRadius = _stream.i32();
					break;
				case 2:
					script.type = Proto::MapScriptType::Timer;
					script.time = _stream.i32();
					break;
				case 3:
					script.type = Proto::MapScriptType::Item;
					break;
				case 4:
					script.type = Proto::MapScriptType::Critter;
					break;
				}

				script.flags = _stream.i32();
				script.scriptId = _stream.i32();
				script.unknown5 = _stream.i32();
				script.oid = _stream.i32();
				script.lvarsOffset = _stream.i32();
				script.lvarsCount = _stream.i32();
				script.unknown9 = _stream.i32();
				script.unknown10 = _stream.i32();
				script.unknown11 = _stream.i32();
				script.unknown12 = _stream.i32();
				script.unknown13 = _stream.i32();
				script.unknown14 = _stream.i32();
				script.unknown15 = _stream.i32();
				script.unknown16 = _stream.i32();

				if (j < count) {
					result.scripts.push_back(script);
				}

				if ((j % 16) == 15) {
					check += _stream.u32();
					_stream.skip(4);
				}
			}

			if (check != count) {
				throw std::runtime_error(QString("Exception in [%1], position [%2]. Error reading scripts: check is incorrect.")
					.arg(_stream.name())
					.arg(_stream.position())
					.toStdString());
			}
		}
	}

	void DataReader::readObjects(Proto::Map& result) {
		const auto count = _stream.u32();

		for (auto& elevation : result.elevations) {
			const auto objectsCount = _stream.u32();
			elevation.objects.resize(objectsCount);
				
			for (size_t i = 0; i < objectsCount; i++) {
				auto& object = elevation.objects[i];
				readObject(object, result);

				object.children.resize(object.invetory.elementsCount);
				for (size_t j = 0; j < object.invetory.elementsCount; j++) {
					auto& inventoryElement = object.children[j];
					inventoryElement.amount = _stream.u32();
					readObject(inventoryElement, result);
				}

			}
		}

	}

	void DataReader::readObject(Proto::MapObject& object, const Proto::Map& map) {
		object.unknown1 = _stream.u32();
		object.position = _stream.u32();
		object.x = _stream.u32();
		object.y = _stream.u32();
		object.sy = _stream.u32();
		object.sx = _stream.u32();
		object.frameId = _stream.u32();
		object.orientation = _stream.u32();
		const auto fid = _stream.u32();
		object.frmTypeId = fid >> 24;
		object.frmId = 0x00FFFFFF & fid;
		object.flags = _stream.u32();
		object.elevation = _stream.u32();
		const auto pid = _stream.u32();
		object.objectTypeId = fid >> 24;
		object.objectId = 0x00FFFFFF & fid;
		object.combatId = _stream.u32();
		object.light.radius = _stream.u32();
		object.light.intensity = _stream.u32();
		object.outlineColor = _stream.u32();

		const auto sid = _stream.i32();
		if (sid != -1) {
			for (const auto& script : map.scripts) {
				if (script.pid == sid) {
					object.mapScriptId = script.scriptId;
					break;
				}
			}
		}

		object.scriptId = _stream.i32();
		object.invetory.elementsCount = _stream.u32();
		object.invetory.slotsCount = _stream.u32();
		object.unknown12 = _stream.u32();
		object.unknown13 = _stream.u32();

		switch ((Proto::ObjectType)object.objectTypeId) {
		case Proto::ObjectType::Item:

			break;
		case Proto::ObjectType::Critter:

			break;
		case Proto::ObjectType::Scenary:

			break;
		case Proto::ObjectType::Wall:

			break;
		case Proto::ObjectType::Tile:

			break;
		case Proto::ObjectType::Misc:

			break;
		default:
			throw std::runtime_error(QString("Exception in [%1], position [%2]. Unknown object type")
				.arg(_stream.name())
				.arg(_stream.position())
				.toStdString());
		}

	}
}
