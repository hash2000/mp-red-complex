#include "Launcher/commands/command_context.h"
#include "Launcher/app_controller.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/services.h"
#include "Launcher/controllers.h"
#include "Libs/Resources/resources.h"

#include <memory>
#include <functional>

class CommandContext::Private {
public:
	Private(CommandContext* parent) : q(parent) { }
	CommandContext* q;

	ApplicationController* applicationController = nullptr;
	CommandContext* globalContext = nullptr;
	std::unique_ptr<Services> services;
	std::unique_ptr<Controllers> controllers;
};

CommandContext::CommandContext(ApplicationController* applicationController,
	CommandContext* globalContext,
	QObject* parent)
: d(std::make_unique<Private>(this))
, QObject(parent) {
	d->globalContext = globalContext;
	d->services = std::make_unique<Services>(applicationController->resources());
	d->controllers = std::make_unique<Controllers>(applicationController, d->services.get());
	d->applicationController = applicationController;
}

CommandContext::~CommandContext() = default;

ApplicationController* CommandContext::applicationController() const {
	return d->applicationController;
}

Services* CommandContext::services() const {
	return d->services.get();
}

Controllers* CommandContext::controllers() const {
	return d->controllers.get();
}

std::unique_ptr<CommandContext> CommandContext::createScopedContext() {
	return std::make_unique<CommandContext>(d->applicationController, globalContext());
}

CommandContext* CommandContext::globalContext() {
	return d->globalContext == nullptr ? this : d->globalContext;
}

bool CommandContext::isGlobalContext() const {
	return d->globalContext == nullptr;
}

void CommandContext::print(const QString& message, const QString& styleClass) {
	QMetaObject::invokeMethod(this, [this, message, styleClass]() {
		emit outputRequested(message, styleClass);
		}, Qt::QueuedConnection);
}

void CommandContext::printSystem(const QString& message) {
	print(message, "system");
}

void CommandContext::printError(const QString& message) {
	print(message, "error");
}

void CommandContext::printSuccess(const QString& message) {
	print(message, "success");
}

void CommandContext::printWarning(const QString& message) {
	print(message, "warning");
}

void CommandContext::setData(const QString& key, const QVariant& value) {
	d->applicationController->resources()->Variables.set(key, value);
}

QVariant CommandContext::data(const QString& key, const QVariant& value) const {
	return d->applicationController->resources()->Variables.get(key, value);
}
