#include "Game/widgets/action_panel/action_panel_widget.h"
#include "Game/controllers/action_panel_controller.h"
#include "Game/styles/items_styles.h"
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QGraphicsEffect>

class ActionPanelWidget::Private {
public:
	Private(ActionPanelWidget* parent) : q(parent) {}
	ActionPanelWidget* q;
	ActionPanelController* controller = nullptr;
	QVBoxLayout* mainLayout = nullptr;
	QWidget* buttonsContainer = nullptr;
	QVBoxLayout* buttonsLayout = nullptr;
	QHash<QString, QToolButton*> buttonMap;  // ID -> QToolButton
	bool isVisible = true;
};

ActionPanelWidget::ActionPanelWidget(ActionPanelController* controller, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->controller = controller;

	// Подключение сигналов контроллера
	connect(d->controller, &ActionPanelController::buttonAdded,
		this, &ActionPanelWidget::onButtonAdded);
	connect(d->controller, &ActionPanelController::buttonRemoved,
		this, &ActionPanelWidget::onButtonRemoved);
	connect(d->controller, &ActionPanelController::allButtonsCleared,
		this, &ActionPanelWidget::onAllButtonsCleared);
	connect(d->controller, &ActionPanelController::buttonsSorted,
		this, &ActionPanelWidget::onButtonsSorted);
	connect(d->controller, &ActionPanelController::panelVisibilityChanged,
		this, &ActionPanelWidget::onPanelVisibilityChanged);

	setupLayout();
}

ActionPanelWidget::~ActionPanelWidget() = default;

void ActionPanelWidget::setupLayout() {
	// Основной layout
	d->mainLayout = new QVBoxLayout(this);
	d->mainLayout->setContentsMargins(0, 0, 0, 0);
	d->mainLayout->setSpacing(2);

	// Контейнер для кнопок
	d->buttonsContainer = new QWidget(this);
	d->buttonsLayout = new QVBoxLayout(d->buttonsContainer);
	d->buttonsLayout->setContentsMargins(0, 0, 0, 0);
	d->buttonsLayout->setSpacing(2);
	d->buttonsLayout->addStretch();  // Кнопки прижимаются к верху

	d->mainLayout->addWidget(d->buttonsContainer);

	// Стиль панели
	setStyleSheet(R"(
		ActionPanelWidget {
			background-color: #f8fafc;
			border-left: 1px solid #e2e8f0;
		}
		QToolButton {
			background-color: transparent;
			border: 1px solid transparent;
			border-radius: 6px;
		}
		QToolButton:hover {
			background-color: #e0f2fe;
			border: 1px solid #bae6fd;
		}
		QToolButton:pressed {
			background-color: #bae6fd;
		}
	)");

	// Фиксированная ширина панели
	setMinimumWidth(ItemsStyles::ICON_SIZE + 16);
	setMaximumWidth(ItemsStyles::ICON_SIZE + 16);
}

QToolButton* ActionPanelWidget::createToolButton(const ActionButtonConfig& config) {
	auto* button = new QToolButton(this);
	button->setIcon(QIcon(config.iconName));
	button->setIconSize(QSize(ItemsStyles::ICON_SIZE, ItemsStyles::ICON_SIZE));
	button->setToolTip(config.toolTip);
	button->setFixedSize(ItemsStyles::ICON_SIZE, ItemsStyles::ICON_SIZE);
	button->setAutoRaise(true);
	button->setProperty("actionId", config.id);
	button->setProperty("actionCommand", config.command);

	// Подключение к слоту
	connect(button, &QToolButton::clicked, this, &ActionPanelWidget::onToolButtonClicked);

	return button;
}

void ActionPanelWidget::onToolButtonClicked() {
	auto* button = qobject_cast<QToolButton*>(sender());
	if (!button) return;

	QString command = button->property("actionCommand").toString();
	if (!command.isEmpty()) {
		d->controller->executeCommand(command);
	}
}

void ActionPanelWidget::onButtonAdded(const ActionButtonConfig& config) {
	// Проверяем, есть ли уже кнопка
	if (d->buttonMap.contains(config.id)) {
		// Обновляем существующую
		auto* oldButton = d->buttonMap[config.id];
		oldButton->setIcon(QIcon(config.iconName));
		oldButton->setToolTip(config.toolTip);
		oldButton->setProperty("actionCommand", config.command);
	} else {
		// Создаём новую
		auto* button = createToolButton(config);
		d->buttonMap[config.id] = button;

		// Вставляем в layout с учётом сортировки
		// Для простоты добавляем перед stretch
		d->buttonsLayout->insertWidget(d->buttonsLayout->count() - 1, button);
	}
}

void ActionPanelWidget::onButtonRemoved(const QString& id) {
	if (d->buttonMap.contains(id)) {
		auto* button = d->buttonMap.take(id);
		button->deleteLater();
	}
}

void ActionPanelWidget::onAllButtonsCleared() {
	for (auto* button : d->buttonMap) {
		button->deleteLater();
	}
	d->buttonMap.clear();
}

void ActionPanelWidget::onButtonsSorted(const std::vector<ActionButtonConfig>& sortedButtons) {
	// Удаляем все кнопки из layout
	for (auto* button : d->buttonMap) {
		d->buttonsLayout->removeWidget(button);
	}

	// Добавляем в отсортированном порядке
	for (const auto& config : sortedButtons) {
		if (d->buttonMap.contains(config.id)) {
			auto* button = d->buttonMap[config.id];
			d->buttonsLayout->insertWidget(d->buttonsLayout->count() - 1, button);
		}
	}
}

void ActionPanelWidget::onPanelVisibilityChanged(bool visible) {
	setPanelVisible(visible);
}

void ActionPanelWidget::setPanelVisible(bool visible) {
	d->isVisible = visible;
	d->buttonsContainer->setVisible(visible);
	setVisible(visible);
}

bool ActionPanelWidget::isPanelVisible() const {
	return d->isVisible;
}
