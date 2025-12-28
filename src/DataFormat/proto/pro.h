#pragma once
#include <QString>
#include <cstdint>

namespace Proto {
// Game entity prototype

enum class ObjectType {
  Item = 0,
  Critter,
  Scenary,
  Wall,
  Tile,
  Misc,
};

struct ProtoHeader {
  ObjectType type;
  uint16_t frameId;
};

enum class ItemFlag : uint32_t {
  Rotate = 0x00000001,
  NoBlock = 0x00000002,
  TransFluid = 0x00000004,
  LightThru = 0x00000008,
  Unused = 0x00000010,
  WallTrans = 0x00000020,
  Lighting = 0x00000040,
  Outline = 0x00000080,
  NoHighlight = 0x00000100,
  Invisible = 0x00000200,
  ShootThru = 0x00000400,
  Unused2 = 0x00000800,
  Flat = 0x00001000,
  ScrollWall = 0x00002000,
  TransRed = 0x00004000,
  TransNone = 0x00008000,
  TransGlass = 0x00010000,
  TransEnergy = 0x00020000,
  TransCursor = 0x00040000
};

enum class MaterialType : uint32_t {
  None = 0,
  Metal = 1,
  Wood = 2,
  Stone = 3,
  Dirt = 4,
  Paper = 5,
  Plastic = 6,
  Water = 7
};

enum class ItemType : uint32_t {
  Armor = 0,
  Container,
  Drug,
  Weapon,
  Ammo,
  Misc,
  Key,
};

enum class SceneryType : uint32_t {
  Door = 0,
  Stairs,
  Elevator,
  Ladder_bottom,
  Ladder_top,
  Generic,
};

struct ProtoItem {
  uint32_t lightDistance;
  uint32_t lightIntensity;
  uint32_t flags;
  uint32_t materialId; // 0 .. 7
  uint32_t size;
  int32_t weight;
  int32_t cost;
  int32_t inventoryFormId; // FID for inventory icon (0 = use fid)
  uint8_t damageDice;      // 1d6 → damage_dice=1, damage_sides=6
  uint8_t damageSides;
  uint8_t damageType;  // DMG_normal=0, DMG_laser=1, ...
  uint8_t animCode;    // ANIM_* (punch=0, swing=1, ...)
  uint32_t scriptId;   // script (0 = none)
  uint32_t subType;    // weapon/ammo/type (depends on item type)
  int32_t maxCharges;  // for ammo/energy cells
  int32_t charges;     // default charges
  uint32_t flagsExt;   // ITEM_FLAG_* extended (e.g. two-handed)
  uint8_t perk;        // required perk (0 = none)
  uint8_t padding[31]; // 112 bytes (total: 4+4*13 + 4 + 4 + 4 + 4 + 1+1+1+1 +
                       // 4+4+4+4 + 4 + 1+31 = 112)
};

struct ProtoCritter {
  uint32_t scriptId;
  uint32_t inventory[10]; // PID предметов (0 = empty)

  int32_t hp;
  int32_t max_hp;
  int32_t stats[45]; // stat[0]=STR, [1]=PER, ..., [6]=LCK,
                     // [7]=Gender, [8]=Current HP (устаревшее),
                     // [12]=XP, [13]=Level, [14]=Skill points,
                     // [24]=Unarmed, [25]=Melee, ..., [44]=Outdoorsman (0–200)

  uint8_t team_num; 		// 0=player, 1=neutral, 2=hostile, 3=merc, 4=robot, 5=animal,
                   		// 6=ghoul
  uint8_t ai_packet; // AI_BEHAVIOR_* (0=none, 1=aggressive, 2=coward, ...)

  uint8_t damage_resist[6]; // DR: [0]=normal, [1]=laser, [2]=fire, [3]=plasma,
                           // [4]=electrical, [5]=emp
  uint8_t damage_thresh[6]; // DT: same order

  uint8_t perk;      // starting perk (PERK_* enum, 0 = none)
  uint8_t gender;    // 0=male, 1=female
  uint16_t padding1; // align

  uint32_t look_at; // script PID to call on "look"
  uint32_t talk_at; // script PID to call on "talk"

  uint32_t critter_flags; // CRITTER_FLAGS_* (0x01=sleeping, 0x02=unconscious,
                          // 0x04=prone, ...)

  uint8_t who_hit_me; // obj PID who last hit (low byte only; full PID in runtime)
  uint8_t padding2[3]; // align

  uint32_t reaction;  // 0..100 (hostile..friendly)
  uint32_t kill_exp;  // XP for killing
  uint32_t body_type; // 0=human, 1=brahmin, 2=rat, 3=deathclaw, ...

  uint32_t head_fid;   // FID for head (0 = use fid)
  uint32_t ai_tactics; // AI_TACTICS_* (0=default, 1=berserk, ...)

  uint8_t radiation_level; // current rad level
  uint8_t poison_level;    // current poison level
  uint16_t padding3;

  uint32_t script_overrides;  // script_id overrides (0 = use script_id)
  uint32_t combat_difficulty; // 0=easy, 1=normal, 2=hard

  uint8_t unused[328]; // до 496 байт (в оригинале — нули или мусор)
};

struct ProtoScentray {
  uint32_t light_distance;
  uint32_t light_intensity;
  uint32_t scriptId;
  uint32_t flags;     // SCENERY_* (e.g. damaged, locked)
  uint32_t elevation; // 0–2 (ground, mid, roof)
};

struct ProtoWall {
	uint32_t scriptId;
  uint32_t flags; // WALL_FLAGS_* (0x01=translucent, 0x02=shoot_through, ...)
  uint32_t elevation;
  uint32_t light_distance;
  uint32_t light_intensity;
  uint32_t material; // MATERIAL_* (same as item)
  uint32_t padding[3]; // до 40 байт
};

struct ProtoTile {
  uint32_t light_distance;
  uint32_t light_intensity;
  uint32_t flags; // TILE_FLAGS_* (0x01=walk_through, 0x02=shoot_through, 0x04=damaging, ...)
};

struct ProtoMisc {
  uint32_t scriptId;
  uint32_t flags; // MISC_FLAGS_* (e.g. 0x01=active, 0x02=looping, ...)
  uint32_t light_distance;
  uint32_t light_intensity;
  uint32_t elevation;
  uint32_t padding[3]; // до 32 байт
};



} // namespace Proto
