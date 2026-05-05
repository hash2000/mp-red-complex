#pragma once
#include "Game/controllers/action_button_config.h"
#include <QWidget>
#include <memory>
#include <vector>

class ActionPanelController;
class ImagesService;
class QToolButton;
class QVBoxLayout;

class ActionPanelWidget : public QWidget {
	Q_OBJECT
public:
	explicit ActionPanelWidget(
		ActionPanelController* controller,
		ImagesService* ImagesService,
		QWidget* parent = nullptr);
	~ActionPanelWidget() override;

	// Скрытие/отображение панели
	void setPanelVisible(bool visible);
	bool isPanelVisible() const;

	// Переключить состояние панели
	void togglePanel();

public slots:
	void onButtonAdded(const ActionButtonConfig& config);
	void onButtonRemoved(const QString& id);
	void onAllButtonsCleared();
	void onButtonsSorted(const std::vector<ActionButtonConfig>& sortedButtons);
	void onPanelVisibilityChanged(bool visible);

private slots:
	void onToolButtonClicked();
	void onToggleButtonClicked();

private:
	void setupLayout();
	QToolButton* createToolButton(const ActionButtonConfig& config);
	void refreshButtons();
	void updateToggleButtonIcon();

	class Private;
	std::unique_ptr<Private> d;
};
