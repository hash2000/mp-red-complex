#pragma once
#include "Content/ConsoleModule/processors/parser/i_ast.h"
#include "Content/ConsoleModule/processors/parser/i_ast_node_compiler.h"

class AstContext : public IAst, public IAstNodeCompiler {
public:

	void compile(std::shared_ptr<ScriptProgram> program) override;
	void setName(const QString& name);
	QString name() const;

private:
	QString _name;
};
