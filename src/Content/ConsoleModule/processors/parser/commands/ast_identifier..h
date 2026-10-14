#pragma once
#include "Content/ConsoleModule/processors/parser/i_ast.h"
#include "Content/ConsoleModule/processors/parser/i_ast_node_compiler.h"

#include <QString>
#include <QVariant>

class AstIdentifier : public IAst, public IAstNodeCompiler {
public:
	void compile(std::shared_ptr<ScriptProgram> program) override;

public:
	QString name;
};
