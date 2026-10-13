#include "Content/ConsoleModule/processors/script_program.h"

void ScriptProgram::addInstruction(std::unique_ptr<Instruction> instruction) {
	_instructions.push_back(std::move(instruction));
}
