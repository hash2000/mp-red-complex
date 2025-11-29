#pragma once
#include "Base/format.h"
#include <QMetaType>
#include <QString>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>

namespace DataFormat::Int {

	enum class ProcedureIType : uint32_t {
		Timed = 0x01,
		Conditional = 0x02,
		Imported = 0x04,
		Exported = 0x08,
		Critical = 0x10,
		Pure = 0x20,
		Inline = 0x40,
	};

	enum class OpCode : uint16_t {
    Noop = 0x8000,
    Push = 0x8001,
    EnterCriticalSection = 0x8002,
    LeaveCriticalSection = 0x8003,
    Jump = 0x8004,
    Call = 0x8005,
    CallAt = 0x8006,
    CallWhen = 0x8007,
    Callstart = 0x8008,
    Exec = 0x8009,
    Spawn = 0x800a,
    Fork = 0x800b,
    A_to_D = 0x800c,
    D_to_A = 0x800d,
    Exit = 0x800e,
    Detach = 0x800f,
    ExitProgram = 0x8010,
    StopProgram = 0x8011,
    FetchGlobal = 0x8012,
    StoreGlobal = 0x8013,
    FetchExternal = 0x8014,
    StoreExternal = 0x8015,
    ExportVariable = 0x8016,
    ExportProcedure = 0x8017,
    Swap = 0x8018,
    Swapa = 0x8019,
    Pop = 0x801a,
    Dup = 0x801b,
    PopReturn = 0x801c,
    PopExit = 0x801d,
    PopAddress = 0x801e,
    PopFlags = 0x801f,
    PopFlagsReturn = 0x8020,
    PopFlagsExit = 0x8021,
    PopFlagsReturnExtern = 0x8022,
    PopFlagsExitExtern = 0x8023,
    PopFlagsReturnValExtern = 0x8024,
    PopFlagsReturnValExit = 0x8025,
    PopFlagsReturnValExitExtern = 0x8026,
    Check_procedure_argument_count = 0x8027,
    Lookup_procedure_by_name = 0x8028,
    PopBase = 0x8029,
    PopToBase = 0x802a,
    PushBase = 0x802b,
    SetGlobal = 0x802c,
    FetchProcedureAddress = 0x802d,
    Dump = 0x802e,
    If = 0x802f,
    While = 0x8030,
    Store = 0x8031,
    Fetch = 0x8032,
    Equal = 0x8033,
    NotEqual = 0x8034,
    LessThanEqual = 0x8035,
    GreaterThanEqual = 0x8036,
    LessThan = 0x8037,
    GreaterThan = 0x8038,
    Add = 0x8039,
    Sub = 0x803a,
    Mul = 0x803b,
    Div = 0x803c,
    Mod = 0x803d,
    And = 0x803e,
    Or = 0x803f,
    BitwiseAnd = 0x8040,
    BitwiseOr = 0x8041,
    BitwiseXor = 0x8042,
    BitwiseNot = 0x8043,
    Floor = 0x8044,
    Not = 0x8045,
    Negate = 0x8046,
    Wait = 0x8047,
    Cancel = 0x8048,
    CancelAll = 0x8049,
    StartCritical = 0x804a,
    EndCritical = 0x804b,
	};

	enum class ProgramType : uint16_t {
    Exited = 0x01,
    x_0x02 = 0x02,
    x_0x04 = 0x04,
    Stopped = 0x08,
    IsWaiting = 0x10,
    x_0x20 = 0x20,
    x_0x40 = 0x40,
    CriticalSection = 0x80,
    x_0x0100 = 0x0100,
	};

	enum class ValueType : uint16_t {
		OpCode = 0x8000,
		Int = 0x4000,
    Float = 0x2000,
    StaticString = 0x1000,
    DynamicString = 0x0800,
	};

	enum class BitMask {
		ValueType = 0xF7FF,
		Int = 0xC001,
		Float = 0xA001,
		StaticString = 0x9001,
		DynamicString = 0x9801,
		Ptr = 0xE001,
	};

	struct Procedure {
    QString name;
    uint32_t flags = 0;
    uint32_t delay = 0; // delay for timed procedures
    uint32_t conditionOffset = 0; // offset of condition in code for conditional procedures
    uint32_t bodyOffset = 0; // procedure body offset from the beginning of the file
    uint32_t argumentsCounter = 0; // number of arguments
		bool isTimed = false;
		bool isConditional = false;
		bool isImported = false;
		bool isExported = false;
		bool isCritical = false;
		bool isPure = false;
		bool isInline = false;
	};

	struct Program {

	};

	struct Header {
    uint32_t procBodyOffset = 0;
    uint32_t procCount = 0;
	};

	struct Programmability {
		Header header;
    std::vector<std::unique_ptr<Procedure>> procedures;
    std::map<unsigned int, std::string> functions;
    std::map<unsigned int, std::string> identifiers;
    std::map<unsigned int, std::string> strings;
	};

	[[nodiscard]] constexpr bool has_flag(std::uint32_t flags, ProcedureIType mask) noexcept {
		using T = std::underlying_type_t<ProcedureIType>;
		return (flags & static_cast<T>(mask)) == static_cast<T>(mask);
	}

	[[nodiscard]] constexpr bool has_flags(std::uint32_t flags, ProcedureIType required_mask) noexcept {
		return has_flag(flags, required_mask);
	}
}

using ProgramOpCode = DataFormat::Int::OpCode;

Q_DECLARE_METATYPE(ProgramOpCode);
