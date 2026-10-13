#include "Content/ConsoleModule/processors/parser/commands/ast_context.h"


void AstContext::setName(const QString& name) {
	_name = name;
}

QString AstContext::name() const {
	return _name;
}

void AstContext::compile(std::shared_ptr<ScriptProgram> program) {

}


