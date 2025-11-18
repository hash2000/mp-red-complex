#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>

namespace Format::Int {

	enum class ProcedureIType {
		Timed = 0x01,
		Conditional = 0x02,
		Imported = 0x04,
		Exported = 0x08,
		Critical = 0x10,
		Pure = 0x20,
		Inline = 0x40,
	};

	struct Procedure {
    std::string name;
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

	struct Programmability {
    std::vector<std::unique_ptr<Procedure>> procedures;
    std::map<unsigned int, std::string> functions;
    std::map<unsigned int, std::string> identifiers;
    std::map<unsigned int, std::string> strings;
	};
}
