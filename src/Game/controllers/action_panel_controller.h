#pragma once
#include "Game/controllers/action_button_config.h"
#include <QObject>
#include <memory>
#include <vector>

class ApplicationController;

class ActionPanelController : public QObject {
	Q_OBJECT
public:
	explicit ActionPanelController(ApplicationController* controller, QObject* parent = nullptr);
	~ActionPanelController() override;

	// Добавление кнопки
	void addButton(const ActionButtonConfig& config);

	// Удаление кнопки по ID
	void removeButton(const QString& id);

	// Удаление всех кнопок
	void clearAllButtons();

	// Сортировка кнопок по viewOrder
	void sortButtons();

	// Скрытие панели
	void hidePanel();

	// Отображение панели
	void showPanel();

	// Выполнение команды
	void executeCommand(const QString& command);

signals:
	void buttonAdded(const ActionButtonConfig& config);
	void buttonRemoved(const QString& id);
	void allButtonsCleared();
	void buttonsSorted(const std::vector<ActionButtonConfig>& sortedButtons);
	void panelVisibilityChanged(bool visible);

private:
	class Private;
	std::unique_ptr<Private> d;
};
