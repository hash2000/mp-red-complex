#include "Game/widgets/items/entities_window.h"
#include "Game/widgets/items/entities_widget.h"
#include "Game/app_controller.h"
#include "Game/commands/command_context.h"
#include "Game/commands/command_processor.h"
#include "ApplicationLayer/items/items_service.h"


class EntitiesWindow::Private {
public:
	Private(EntitiesWindow* parent)
	: q(parent) {
	}

	EntitiesWindow* q;
	ItemsService* service;
	EntitiesWidget* widget;
	CommandProcessor* cmds = nullptr;
};

EntitiesWindow::EntitiesWindow(ItemsService* service, const QString& id, QWidget* parent)
: d(std::make_unique<Private>(this))
, MdiChildWindow(id, parent) {
	d->service = service;
	d->widget = new EntitiesWidget(service, this);

	connect(d->widget, &EntitiesWidget::itemCreateRequested,
		this, &EntitiesWindow::onItemCreateRequested);

	setWidget(d->widget);
}

EntitiesWindow::~EntitiesWindow() = default;


bool EntitiesWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		for (const auto& item : d->service->entities()) {
			d->widget->addItemEntity(item);
		}

		d->cmds = context->applicationController()->commandProcessor();

		return true;
	}

	return false;
}

void EntitiesWindow::onItemCreateRequested(const QString& itemId) {
	if (!d->cmds) {
		return;
	}
}
