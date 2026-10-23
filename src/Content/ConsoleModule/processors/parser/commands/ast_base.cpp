#include "Content/ConsoleModule/processors/parser/commands/ast_base.h"

class AstBase::Private {
public:
	Private(AstBase* parent) : q(parent) { }
	AstBase* q;

	SourceLocation location;
	IAstGroupegCollection children;
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

void AstBase::setChild(const QString& type, IAstPtr child) {
	auto it = d->children.find(type);
	if (it == d->children.end()) {
		it = d->children.emplace(type, IAstCollection()).first;
	}

	it->second.emplace_back(child);
}

void AstBase::setChildren(const QString& type, const IAstCollection& children) {
	auto it = d->children.find(type);
	if (it == d->children.end()) {
		it = d->children.emplace(type, IAstCollection()).first;
	}

	for (const auto& item : children) {
		it->second.emplace_back(item);
	}
}

size_t AstBase::childTypesCount() const {
	return d->children.size();
}

std::list<QString> AstBase::childTypes() const {
	std::list<QString> result;
	for (const auto& i : d->children) {
		result.push_back(i.first);
	}

	return std::move(result);
}


IAstCollection AstBase::children(const QString& type) const {
	const auto it = d->children.find(type);
	if (it == d->children.end()) {
		return IAstCollection();
	}

	return it->second;
}
