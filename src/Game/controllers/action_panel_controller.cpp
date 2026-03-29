#include "Game/controllers/action_panel_controller.h"
#include "Game/app_controller.h"
#include <algorithm>

class ActionPanelController::Private {
public:
	Private(ActionPanelController* parent) : q(parent) {}
	ActionPanelController* q;
	ApplicationController* controller = nullptr;
	std::vector<ActionButtonConfig> buttons;
	bool isVisible = true;
};

ActionPanelController::ActionPanelController(ApplicationController* controller, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->controller = controller;
}

ActionPanelController::~ActionPanelController() = default;

void ActionPanelController::addButton(const ActionButtonConfig& config) {
	// Проверяем, есть ли уже кнопка с таким ID
	auto it = std::find_if(d->buttons.begin(), d->buttons.end(),
		[&config](const ActionButtonConfig& btn) {
			return btn.id == config.id;
		});

	if (it != d->buttons.end()) {
		// Обновляем существующую кнопку
		*it = config;
	} else {
		// Добавляем новую
		d->buttons.push_back(config);
	}

	emit buttonAdded(config);
}

void ActionPanelController::removeButton(const QString& id) {
	auto it = std::find_if(d->buttons.begin(), d->buttons.end(),
		[&id](const ActionButtonConfig& btn) {
			return btn.id == id;
		});

	if (it != d->buttons.end()) {
		d->buttons.erase(it);
		emit buttonRemoved(id);
	}
}

void ActionPanelController::clearAllButtons() {
	d->buttons.clear();
	emit allButtonsCleared();
}

void ActionPanelController::sortButtons() {
	std::sort(d->buttons.begin(), d->buttons.end(),
		[](const ActionButtonConfig& a, const ActionButtonConfig& b) {
			return a.viewOrder < b.viewOrder;
		});

	emit buttonsSorted(d->buttons);
}

void ActionPanelController::hidePanel() {
	d->isVisible = false;
	emit panelVisibilityChanged(false);
}

void ActionPanelController::showPanel() {
	d->isVisible = true;
	emit panelVisibilityChanged(true);
}

void ActionPanelController::executeCommand(const QString& command) {
	if (d->controller) {
		d->controller->executeCommand(command);
	}
}
