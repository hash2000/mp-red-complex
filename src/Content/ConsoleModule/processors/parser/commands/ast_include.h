#pragma once
#include "Content/ConsoleModule/processors/parser/i_ast.h"
#include "Content/ConsoleModule/processors/parser/i_ast_node_compiler.h"

class AstInclude : public IAst, public IAstNodeCompiler {
public:
	void compile(std::shared_ptr<ScriptProgram> program) override;
	void setPath(const QString& path);
	QString path() const;

private:
	QString _path;
};
