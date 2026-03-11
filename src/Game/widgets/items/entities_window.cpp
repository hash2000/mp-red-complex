#include "Game/widgets/items/entities_window.h"
#include "Game/widgets/items/entities_widget.h"
#include "Game/widgets/items/item_create_widget.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/commands/command_context.h"
#include "Game/commands/command_processor.h"
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

	// Получаем ID целевого инвентаря (например, первый доступный или активный)
	// В будущем можно сделать выбор инвентаря через UI
	const auto services = d->controller->services();
	const auto inventoriesService = services->inventoriesService();
	const auto inventories = inventoriesService->getAllIds();
	
	if (inventories.empty()) {
		qWarning() << "EntitiesWindow::onItemCreateRequested: no inventories available";
		return;
	}

	const QString targetInventoryId = QString::fromUtf8(inventories.front().toByteArray());

	// Создаём и показываем диалог создания предмета
	auto createDialog = new ItemCreateWidget(*entity, d->service, targetInventoryId, this);
	connect(createDialog, &ItemCreateWidget::itemCreated, this, [this, inventoriesService, targetInventoryId](const QString& entityId) {
		// Создаём предмет в инвентаре через сервис
		const auto newItem = d->service->duplicate(entityId);
		if (!newItem) {
			qWarning() << "Failed to create item:" << entityId;
			return;
		}

		// Получаем сервис инвентаря
		auto inventoryService = static_cast<InventoryService*>(
			inventoriesService->placementService(QUuid::fromString(targetInventoryId), false));
		
		if (!inventoryService) {
			qWarning() << "Failed to get inventory service for:" << targetInventoryId;
			return;
		}

		// Создаём ItemMimeData для нового предмета
		ItemMimeData mimeData(*newItem);
		
		// Пытаемся найти свободное место в инвентаре
		const auto freeSpace = inventoryService->findFreeSpace(mimeData, true);
		if (!freeSpace.has_value()) {
			qWarning() << "No free space in inventory for item:" << entityId;
			return;
		}

		// Размещаем предмет в инвентаре
		mimeData.x = freeSpace->x();
		mimeData.y = freeSpace->y();
		
		if (inventoryService->applyDublicateFromItem(mimeData)) {
			qInfo() << "Item placed in inventory:" << newItem->id << "at" << mimeData.x << "," << mimeData.y;
		}
		else {
			qWarning() << "Failed to place item in inventory:" << newItem->id;
		}
		});

	createDialog->show();
}
