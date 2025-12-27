#pragma once
#include <cstdint>
#include <QString>
#include <vector>

namespace Proto {

	struct MapObject {
		uint32_t unknown1;
		uint32_t position;
		uint32_t x;
		uint32_t y;
		uint32_t sx;
		uint32_t sy;

		// Frame number.This is the frame index of the frame in the FRM
		// file this is currently being displayed.
		uint32_t frameId;

		uint32_t orientation;
		uint32_t frmTypeId;
		uint32_t frmId;
		uint32_t flags;
		uint32_t elevation;
		uint32_t objectTypeId;
		uint32_t objectId;

		// (Only for in-battle .SAV) -1 for normal objects
		int32_t combatId;
		struct {
			uint32_t radius;
			uint32_t intensity;
		} light;

		// Outline color. (Only for in - battle.SAV)
		//	0x0 - no outline
		//	0x1 - red
		//	0x20 - yellow
		uint32_t outlineColor;

		uint32_t mapScriptId;

		//Script filename is found in LST file script.lst at index id.
		// A value of -1 means no script.
		int32_t scriptId;

		struct {
			// Number of map objects in this object's inventory.
			// If this is non zero, then after reading this map object,
			// the objects in the inventory must be read.
			// The objects in the inventory are map objects as well,
			// and they follow this object, being preceeded by a 4 byte
			// integer being the count of this map object in the inventory.
			uint32_t elementsCount;

			// Maximum number of slots in critter inventory.
			uint32_t slotsCount;
		} invetory;

		uint32_t unknown12;
		uint32_t unknown13;

		uint32_t amount;
		std::vector<MapObject> children;
	};

	struct Elevation {
		std::vector<uint16_t> floors;
		std::vector<uint16_t> roofs;
		std::vector<MapObject> objects;
	};

	enum class MapScriptType {
		System = 0,
		Spatial,
		Timer,
		Item,
		Critter,
	};

	struct MapScript {
		MapScriptType type;
		int32_t pid{ 0 };
		int32_t nextScriptId{ -1 };
		int32_t spatialTile{ 0 };
		int32_t spatialRadius{ 0 };
		int32_t time{ 0 };
		int32_t flags{ 0 };
		int32_t scriptId{ -1 };
		int32_t unknown5;
		int32_t oid;
		int32_t lvarsOffset;
		int32_t lvarsCount;
		int32_t unknown9;
		int32_t unknown10;
		int32_t unknown11;
		int32_t unknown12;
		int32_t unknown13;
		int32_t unknown14;
		int32_t unknown15;
		int32_t unknown16;
	};

	struct Map {
		uint32_t version{ 0 };
		QString name;
		int32_t position;
		int32_t elevation;
		int32_t orientation;
		int32_t lvarsSize;
		int32_t mvarsSize;
		int32_t scriptId;
		int32_t flags;
		int32_t elevationLevel;
		int32_t darkness; // maybe ubused
		int32_t id;

		// Time since the epoch.Number of time ticks since the epoch.
		// A time tick is equivalent to 0.1 seconds in game time.The epoch
		// for Fallout 1 is "5 December 2161 00:00am", and for Fallout 2 "25 July 2241 00:00am".
		uint32_t epochTime;
		std::vector<int32_t> mvars;
		std::vector<int32_t> lvars;
		std::vector<Elevation> elevations;
		std::vector<MapScript> scripts;
	};

} // namespace Proto
