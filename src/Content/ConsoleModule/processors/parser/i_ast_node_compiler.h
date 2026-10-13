#pragma once
#include "Content/ConsoleModule/processors/script_program.h"

class IAstNodeCompiler {
public:
	virtual ~IAstNodeCompiler() = default;
	virtual void compile(std::shared_ptr<ScriptProgram> program) = 0;
};
