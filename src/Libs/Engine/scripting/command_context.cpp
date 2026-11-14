#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/services/services_registry.h"

#include <QDebug>
#include <QList>
#include <QMap>

class CommandContext::Private {
public:
	Private(CommandContext* parent) : q(parent) { }
	CommandContext* q;

	CommandController* commandController = nullptr;
	QMap<QString, std::weak_ptr<CommandContext>> contexts;
	std::shared_ptr<CommandContext> parentContext;
	QString name;
	QString nameSpace;

	QMap<QString, QVariant> variables;
	QMap<QString, QString> variableMimeTypes;
	CommandContext::VariablesMode variablesMode = CommandContext::VariablesMode::Lazy;
};

CommandContext::CommandContext(CommandController* commandController, std::shared_ptr<CommandContext> parentContext, const QString& name, QObject* parent)
: d(std::make_unique<Private>(this))
, QObject(parent) {
	d->commandController = commandController;
	d->parentContext = parentContext;
	d->name = name;

	if (parentContext) {
		auto parentName = QString();
		if (!parentContext->isGlobalContext()) {
			parentName = parentContext->nameSpace();
		}
		d->nameSpace = QString("%1.%2")
			.arg(parentName)
			.arg(name);
	}
	else {
		d->nameSpace = ".";
		d->name = QString();
	}
}

CommandContext::~CommandContext() = default;

CommandController* CommandContext::commandController() const {
	return d->commandController;
}

QString CommandContext::name() const {
	return d->name;
}

QString CommandContext::nameSpace() const {
	return d->nameSpace;
}

void CommandContext::registerContext(const QString& ns, std::weak_ptr<CommandContext> context) {
	if (d->contexts.contains(ns)) {
		qWarning() << "CommandContext::registerNamespace:" << ns << "allready defined. Revrite.";
	}

	d->contexts[ns] = context;
}

std::shared_ptr<CommandContext> CommandContext::createScopedContext(const QString& name) {
	auto self = shared_from_this();
	auto scoped = std::make_shared<CommandContext>(d->commandController, self, name);
	d->commandController->globalContext()->registerContext(scoped->nameSpace(), scoped);
	return scoped;
}

std::shared_ptr<CommandContext> CommandContext::parentContext() const {
	return d->parentContext;
}

std::shared_ptr<CommandContext> CommandContext::globalContext() const {
	return d->commandController->globalContext();
}

bool CommandContext::isGlobalContext() const {
	return !d->parentContext;
}

std::shared_ptr<CommandContext> CommandContext::context(const QString& ns) const {
	if (ns == ".") {
		return globalContext();
	}

	if (isGlobalContext()) {
		const auto it = d->contexts.constFind(ns);
		if (it == d->contexts.constEnd()) {
			return nullptr;
		}

		auto locked = it->lock();
		if (!locked) {
			return nullptr;
		}

		return locked;
	}

	return globalContext()->context(ns);
}

QStringList CommandContext::nameSpaces() const {
	if (isGlobalContext()) {
		QStringList result;
		result << ".";
		result << d->contexts.keys();
		return result;
	}

	return globalContext()->nameSpaces();
}

void CommandContext::setVariablesMode(VariablesMode mode) {
	if (d->variablesMode == mode) {
		return;
	}

	d->variablesMode = mode;

	if (mode == VariablesMode::Atomic) {
		snapshotParentVariables();
	}
}

CommandContext::VariablesMode CommandContext::variablesMode() const {
	return d->variablesMode;
}

bool CommandContext::setVariable(const QString& name, const QVariant& value) {
	if (d->variablesMode == VariablesMode::ReadOnly) {
		qWarning() << "CommandContext::setVariable: variable" << name
			<< "is read-only in" << d->nameSpace;
		return false;
	}

	// mutable: меняем общую переменную базового контекста, если она там есть.
	if (d->variablesMode == VariablesMode::Mutable &&
		!d->variables.contains(name) &&
		d->parentContext &&
		d->parentContext->hasVariable(name)) {
		return d->parentContext->setVariable(name, value);
	}

	d->variables.insert(name, value);
	return true;
}

bool CommandContext::hasVariable(const QString& name) const {
	if (d->variables.contains(name)) {
		return true;
	}

	if (d->variablesMode == VariablesMode::Atomic || !d->parentContext) {
		return false;
	}

	return d->parentContext->hasVariable(name);
}

QVariant CommandContext::variable(const QString& name, const QVariant& defaultValue) const {
	const auto it = d->variables.constFind(name);
	if (it != d->variables.constEnd()) {
		return it.value();
	}

	if (d->variablesMode == VariablesMode::Atomic || !d->parentContext) {
		return defaultValue;
	}

	return d->parentContext->variable(name, defaultValue);
}

QVariantMap CommandContext::variables() const {
	QVariantMap result;

	if (d->variablesMode != VariablesMode::Atomic && d->parentContext) {
		result = d->parentContext->variables();
	}

	for (auto it = d->variables.cbegin(); it != d->variables.cend(); ++it) {
		result.insert(it.key(), it.value());
	}

	return result;
}

QStringList CommandContext::variableNames() const {
	return variables().keys();
}

QVariantMap CommandContext::ownVariables() const {
	return d->variables;
}

void CommandContext::setVariableMimeType(const QString& name, const QString& mimeType) {
	if (mimeType.isEmpty()) {
		d->variableMimeTypes.remove(name);
		return;
	}

	d->variableMimeTypes.insert(name, mimeType);
}

QString CommandContext::variableMimeType(const QString& name, const QString& defaultValue) const {
	const auto it = d->variableMimeTypes.constFind(name);
	if (it != d->variableMimeTypes.constEnd()) {
		return it.value();
	}

	if (d->variablesMode == VariablesMode::Atomic || !d->parentContext) {
		return defaultValue;
	}

	return d->parentContext->variableMimeType(name, defaultValue);
}

void CommandContext::snapshotParentVariables() {
	if (!d->parentContext) {
		return;
	}

	const auto inherited = d->parentContext->variables();
	for (auto it = inherited.cbegin(); it != inherited.cend(); ++it) {
		if (!d->variables.contains(it.key())) {
			d->variables.insert(it.key(), it.value());
		}

		if (d->variableMimeTypes.contains(it.key())) {
			continue;
		}

		const auto mimeType = d->parentContext->variableMimeType(it.key());
		if (!mimeType.isEmpty()) {
			d->variableMimeTypes.insert(it.key(), mimeType);
		}
	}
}
