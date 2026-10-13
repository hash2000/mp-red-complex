#include "Content/ConsoleModule/processors/parser/commands/ast_include.h"

void AstInclude::setPath(const QString& path) {
	_path = path;
}

QString AstInclude::path() const {
	return _path;
}

void AstInclude::compile(std::shared_ptr<ScriptProgram> program) {

}
