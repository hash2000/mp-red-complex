#include "Game/widgets/items/entities_window.h"
#include "Game/widgets/items/entities_widget.h"
#include "Game/widgets/items/item_create_widget.h"
#include "Game/app_controller.h"
#include "Game/commands/command_context.h"
#include "Game/commands/command_processor.h"
#include "Game/controllers/windows_controller.h"
#include "Game/mdi_child_window.h"
#include "Game/services.h"
#include "Game/controllers.h"
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/items/item_mime_data.h"


class EntitiesWindow::Private {
public:
	Private(EntitiesWindow* parent)
	: q(parent) {
	}

	EntitiesWindow* q;
	ItemsService* service;
	EntitiesWidget* widget;
	ApplicationController* controller = nullptr;
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

		d->controller = context->applicationController();

		return true;
	}

	return false;
}

void EntitiesWindow::onItemCreateRequested(const QString& itemId) {
	if (!d->controller) {
		return;
	}

	// Получаем сущность предмета по ID
	const auto entity = d->service->entityById(itemId);
	if (!entity) {
		qWarning() << "EntitiesWindow::onItemCreateRequested: entity not found:" << itemId;
		return;
	}

	// Получаем сервисы
	const auto services = d->controller->services();
	const auto inventoriesService = services->inventoriesService();
	const auto windowsController = d->controller->controllers()->windowsController();

	// Собираем список всех открытых инвентарей
	QStringList openInventoryIds;
	QString lastActiveInventoryId;
	
	const auto windows = windowsController->windowEntries();
	for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
		const auto window = it->first.data();
		if (window && window->windowType() == "inventory") {
			const QString windowId = window->windowId();
			openInventoryIds.prepend(windowId);  // Добавляем в начало, чтобы последний был первым
			if (lastActiveInventoryId.isEmpty()) {
				lastActiveInventoryId = windowId;
			}
		}
	}

	// Создаём и показываем диалог создания предмета
	auto createDialog = new ItemCreateWidget(*entity, d->service, inventoriesService, this);
	
	// Передаём список доступных инвентарей
	createDialog->setAvailableInventories(openInventoryIds, lastActiveInventoryId);
	
	connect(createDialog, &ItemCreateWidget::itemCreated, this, [this](
		const QString& entityId, int count, const QString& inventoryId) {
			d->controller->executeCommandByName("items-create", QStringList{ entityId, QString::number(count), inventoryId});
	});

	createDialog->show();
}
