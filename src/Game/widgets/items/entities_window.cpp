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
	ItemsService* itemsService;
	EntitiesWidget* widget;
	ApplicationController* controller = nullptr;
	InventoriesService* inventoriesService = nullptr;
};

EntitiesWindow::EntitiesWindow(const QString& id, QWidget* parent)
: d(std::make_unique<Private>(this))
, MdiChildWindow(id, parent) {
}

EntitiesWindow::~EntitiesWindow() = default;


bool EntitiesWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		d->controller = context->applicationController();
		auto services = d->controller->services();
		d->inventoriesService = services->inventoriesService();
		d->itemsService = services->itemsService();
		d->widget = new EntitiesWidget(d->itemsService, nullptr, this);
		d->widget->setInventoriesService(d->inventoriesService);

		for (const auto& item : d->itemsService->entities()) {
			d->widget->addItemEntity(item);
		}

		setWidget(d->widget);

		connect(d->widget, &EntitiesWidget::itemCreateRequested, this, &EntitiesWindow::onItemCreateRequested);
		connect(d->widget, &EntitiesWidget::inventorySelectionRequested, this, &EntitiesWindow::onInventorySelectionRequested);
		onInventorySelectionRequested();

		return true;
	}

	return false;
}

void EntitiesWindow::onItemCreateRequested(const QString& itemId, const QString& inventoryId) {
	if (!d->controller) {
		return;
	}

	// Получаем сущность предмета по ID
	const auto entity = d->itemsService->entityById(itemId);
	if (!entity) {
		qWarning() << "EntitiesWindow::onItemCreateRequested: entity not found:" << itemId;
		return;
	}

	// Проверяем, что инвентарь указан
	if (inventoryId.isEmpty()) {
		qWarning() << "EntitiesWindow::onItemCreateRequested: inventoryId is empty for item:" << itemId;
		return;
	}

	// Создаём и показываем диалог создания предмета
	auto createDialog = new ItemCreateWidget(*entity, d->itemsService, inventoryId, this);

	connect(createDialog, &ItemCreateWidget::itemCreated, this, [this](const QString& entityId, int count, const QString& invId) {
		const auto cmdEntityId = QString("entity:%1").arg(entityId);
		const auto cmdCount = QString("count:%1").arg(QString::number(count));
		const auto cmdInventory = QString("inventory:%1").arg(invId);

		d->controller->executeCommandByName("items-create", QStringList{ cmdEntityId, cmdCount, cmdInventory });
	});

	createDialog->show();
}

void EntitiesWindow::onInventorySelectionRequested() {
	if (!d->controller || !d->widget || !d->inventoriesService) {
		return;
	}

	const auto windowsController = d->controller->controllers()->windowsController();

	// Собираем список всех открытых инвентарей
	QStringList openInventoryIds;
	QString lastActiveInventoryId;

	const auto windows = windowsController->windowEntries();
	for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
		const auto window = it->first.data();
		if (window && window->windowType() == "inventory") {
			const QString windowId = window->windowId();
			openInventoryIds.prepend(windowId);
			if (lastActiveInventoryId.isEmpty()) {
				lastActiveInventoryId = windowId;
			}
		}
	}

	// Передаём список доступных инвентарей в виджет
	d->widget->setAvailableInventories(openInventoryIds, lastActiveInventoryId);
}
