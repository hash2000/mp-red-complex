#pragma once
#include "Libs/Engine/scripting/instruction.h"
#include <memory>
#include <list>

class ScriptProgram {
public:
	virtual ~ScriptProgram() = default;

	void addInstruction(std::unique_ptr<Instruction> instruction);

private:
	std::list<std::unique_ptr<Instruction>> _instructions;
};
