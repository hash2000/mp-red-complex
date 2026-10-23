#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Libs/Engine/services/services_registry.h"

#include <QDebug>
#include <QList>
#include <QMap>

class CommandContext::Private {
public:
	Private(CommandContext* parent) : q(parent) { }
	CommandContext* q;

	CommandController* commandController = nullptr;
	QList<std::weak_ptr<CommandContext>> scopedList;
	QMap<const QString, std::weak_ptr<CommandContext>> contexts;
	std::shared_ptr<CommandContext> parentContext;
	QString name;
	QString nameSpace;
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
	d->scopedList.push_back(scoped->weak_from_this());
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
