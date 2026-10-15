#pragma once
#include "Content/ConsoleModule/processors/parser/commands/ast_base.h"
#include "Content/ConsoleModule/processors/parser/i_ast_node_compiler.h"

class AstContext : public AstBase, public IAstNodeCompiler {
public:
	void compile(std::shared_ptr<ScriptProgram> program) override;
};
