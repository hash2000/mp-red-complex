#pragma once
#include "Game/controllers/action_button_config.h"
#include <QWidget>
#include <memory>
#include <vector>

class ActionPanelController;
class TexturesService;
class QToolButton;
class QVBoxLayout;

class ActionPanelWidget : public QWidget {
	Q_OBJECT
public:
	explicit ActionPanelWidget(
		ActionPanelController* controller,
		TexturesService* texturesService,
		QWidget* parent = nullptr);
	~ActionPanelWidget() override;

	// Скрытие/отображение панели
	void setPanelVisible(bool visible);
	bool isPanelVisible() const;

public slots:
	void onButtonAdded(const ActionButtonConfig& config);
	void onButtonRemoved(const QString& id);
	void onAllButtonsCleared();
	void onButtonsSorted(const std::vector<ActionButtonConfig>& sortedButtons);
	void onPanelVisibilityChanged(bool visible);

private slots:
	void onToolButtonClicked();

private:
	void setupLayout();
	QToolButton* createToolButton(const ActionButtonConfig& config);
	void refreshButtons();

	class Private;
	std::unique_ptr<Private> d;
};
