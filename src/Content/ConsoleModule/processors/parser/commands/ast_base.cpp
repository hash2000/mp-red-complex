#include "Content/ConsoleModule/processors/parser/commands/ast_base.h"

class AstBase::Private {
public:
	Private(AstBase* parent) : q(parent) { }
	AstBase* q;

	SourceLocation location;
	IAstCollection children;
	QVariantMap parameters;
};

AstBase::AstBase()
	: d(std::make_unique<Private>(this)) {
}

AstBase::~AstBase() = default;

void AstBase::setLocation(const SourceLocation& loc) {
	d->location = loc;
}

SourceLocation AstBase::location() const {
	return d->location;
}

void AstBase::setParameter(const QString& name, const QVariant& value) {
	d->parameters[name] = value;
}

QVariant AstBase::parameter(const QString& name) const {
	return d->parameters[name];
}

void AstBase::setChild(IAstPtr child) {
	d->children.emplace_back(child);
}

IAstCollection& AstBase::children() const {
	return d->children;
}
