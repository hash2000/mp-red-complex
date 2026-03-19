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
	InventoriesService* inventoriesService = nullptr;
};

EntitiesWindow::EntitiesWindow(ItemsService* service, const QString& id, QWidget* parent)
: d(std::make_unique<Private>(this))
, MdiChildWindow(id, parent) {
	d->service = service;
	
	// Виджет будет инициализирован с inventoriesService в handleCommand
	d->widget = new EntitiesWidget(service, nullptr, this);

	connect(d->widget, &EntitiesWidget::itemCreateRequested,
		this, &EntitiesWindow::onItemCreateRequested);
	
	connect(d->widget, &EntitiesWidget::inventorySelectionRequested,
		this, &EntitiesWindow::onInventorySelectionRequested);

	setWidget(d->widget);
}

EntitiesWindow::~EntitiesWindow() = default;


bool EntitiesWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		for (const auto& item : d->service->entities()) {
			d->widget->addItemEntity(item);
		}

		d->controller = context->applicationController();
		d->inventoriesService = d->controller->services()->inventoriesService();

		// Инициализируем виджет сервисом инвентарей
		d->widget->setInventoriesService(d->inventoriesService);
		
		// Сразу загружаем список доступных инвентарей
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
	const auto entity = d->service->entityById(itemId);
	if (!entity) {
		qWarning() << "EntitiesWindow::onItemCreateRequested: entity not found:" << itemId;
		return;
	}

	// Проверяем, что инвентарь указан
	if (inventoryId.isEmpty()) {
		qWarning() << "EntitiesWindow::onItemCreateRequested: inventoryId is empty for item:" << itemId;
		return;
	}

	// Получаем сервисы
	const auto services = d->controller->services();
	const auto itemsService = d->service;
	const auto inventoriesService = services->inventoriesService();

	// Создаём и показываем диалог создания предмета
	auto createDialog = new ItemCreateWidget(*entity, itemsService, inventoryId, this);

	connect(createDialog, &ItemCreateWidget::itemCreated, this, [this](
		const QString& entityId, int count, const QString& invId) {
			d->controller->executeCommandByName("items-create", QStringList{ entityId, QString::number(count), invId});
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
