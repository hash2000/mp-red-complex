#include "Game/commands/command_context.h"
#include "Game/app_controller.h"
#include "Game/controllers/windows_controller.h"
#include "Game/mdi_child_window.h"
#include <QMdiSubWindow>
#include <QMetaObject>
#include <QVariantMap>
#include <memory>
#include <functional>

class CommandContext::Private {
public:
	Private(CommandContext* parent, ApplicationController* controller)
	: q(parent)
	, controller(controller){
	}

	CommandContext* q;

	ApplicationController* controller;
	QVariantMap userData;
};

CommandContext::CommandContext(ApplicationController* controller, QObject* parent)
: d(new Private(this, controller))
, QObject(parent){
}

CommandContext::~CommandContext() = default;

ApplicationController* CommandContext::applicationController() const {
	return d->controller;
}

QMdiArea* CommandContext::mdiArea() const {
	return d->controller ? d->controller->windowsController()->mdiArea() : nullptr;
}

MdiChildWindow* CommandContext::activeWindow() const {
	if (!d->controller || !d->controller->windowsController()->mdiArea()) {
		return nullptr;
	}

	auto subWindow = d->controller->windowsController()->mdiArea()->activeSubWindow();

	// 🔑 Правильное использование qobject_cast с полным определением класса
	if (subWindow && subWindow->widget()) {
		return qobject_cast<MdiChildWindow*>(subWindow->widget());
	}

	return nullptr;
}

MdiChildWindow* CommandContext::findWindowById(const QString& id) const {
	return d->controller ? d->controller->windowsController()->findWindowById(id) : nullptr;
}

QList<MdiChildWindow*> CommandContext::allWindows() const {
	QList<MdiChildWindow*> result;
	if (!d->controller || !d->controller->windowsController()->mdiArea()) return result;

	for (auto subWindow : d->controller->windowsController()->mdiArea()->subWindowList()) {
		if (auto window = qobject_cast<MdiChildWindow*>(subWindow->widget())) {
			result.append(window);
		}
	}
	return result;
}

void CommandContext::print(const QString& message, const QString& styleClass) {
	QMetaObject::invokeMethod(this, [this, message, styleClass]() {
		emit outputRequested(message, styleClass);
		}, Qt::QueuedConnection);
}

void CommandContext::printError(const QString& message) {
	print(message, "error");
}

void CommandContext::printSuccess(const QString& message) {
	print(message, "success");
}

void CommandContext::setData(const QString& key, const QVariant& value) {
	d->userData[key] = value;
}

QVariant CommandContext::data(const QString& key) const {
	return d->userData.value(key);
}
