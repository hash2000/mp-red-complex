#include "Game/widgets/action_panel/action_panel_widget.h"
#include "Game/controllers/action_panel_controller.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "Game/styles/items_styles.h"
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QGraphicsEffect>
#include <QStackedLayout>

class ActionPanelWidget::Private {
public:
	Private(ActionPanelWidget* parent) : q(parent) {}

	ActionPanelWidget* q;
	ActionPanelController* controller = nullptr;
	TexturesService* texturesService = nullptr;
	QHBoxLayout* mainLayout = nullptr;
	QWidget* buttonsContainer = nullptr;
	QVBoxLayout* buttonsLayout = nullptr;
	QWidget* toggleContainer = nullptr;      // Контейнер кнопки переключения
	QVBoxLayout* toggleLayout = nullptr;
	QToolButton* toggleButton = nullptr;     // Кнопка сворачивания/разворачивания
	QHash<QString, QToolButton*> buttonMap;  // ID -> QToolButton
	bool isVisible = false;  // По умолчанию панель скрыта (стрелка вправо)
};

ActionPanelWidget::ActionPanelWidget(
	ActionPanelController* controller,
	TexturesService* texturesService,
	QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->controller = controller;
	d->texturesService = texturesService;

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
	// Основной layout - горизонтальный, чтобы разделить кнопки и переключатель
	d->mainLayout = new QHBoxLayout(this);
	d->mainLayout->setContentsMargins(0, 40, 0, 40);
	d->mainLayout->setSpacing(2);

	// Контейнер для кнопок действий (левая часть)
	d->buttonsContainer = new QWidget(this);
	d->buttonsLayout = new QVBoxLayout(d->buttonsContainer);
	d->buttonsLayout->setContentsMargins(0, 0, 0, 0);
	d->buttonsLayout->setSpacing(8);  // Расстояние между кнопками 8px
	d->buttonsLayout->addStretch();  // Кнопки прижимаются к верху

	d->mainLayout->addWidget(d->buttonsContainer, 1);  // stretch = 1

	// Контейнер кнопки переключения (правая часть, фиксированная ширина)
	d->toggleContainer = new QWidget(this);
	d->toggleContainer->setFixedWidth(16);  // Фиксированная ширина для колонки переключателя
	d->toggleLayout = new QVBoxLayout(d->toggleContainer);
	d->toggleLayout->setContentsMargins(0, 0, 0, 0);
	d->toggleLayout->setSpacing(0);
	d->toggleLayout->addStretch();  // Пружина сверху

	// Кнопка переключения (внизу правой колонки)
	d->toggleButton = new QToolButton(this);
	d->toggleButton->setFixedSize(16, 16);
	d->toggleButton->setAutoRaise(true);
	connect(d->toggleButton, &QToolButton::clicked, this, &ActionPanelWidget::onToggleButtonClicked);
	updateToggleButtonIcon();

	// Стиль кнопки переключения
	d->toggleButton->setStyleSheet(R"(
		QToolButton {
			background-color: transparent;
			border: none;
			border-radius: 3px;
			color: #64748b;
			font-size: 10px;
		}
		QToolButton:hover {
			background-color: #e2e8f0;
			color: #334155;
		}
		QToolButton:pressed {
			background-color: #cbd5e1;
		}
	)");

	d->toggleLayout->addWidget(d->toggleButton, 0, Qt::AlignHCenter);
	d->toggleLayout->addStretch();  // Пружина снизу (для центрирования)

	d->mainLayout->addWidget(d->toggleContainer, 0);  // stretch = 0, фиксированная ширина

	// Стиль панели
	setStyleSheet(R"(
		ActionPanelWidget {
			background-color: #f8fafc;
			border-left: 1px solid #e2e8f0;
		}
		/* Кнопки действий */
		ActionPanelWidget QToolButton[actionId] {
			background-color: transparent;
			border: 1px solid transparent;
			border-radius: 3px;
		}
		ActionPanelWidget QToolButton[actionId]:hover {
			background-color: transparent;
			border: 1px solid #60a5fa;
		}
		ActionPanelWidget QToolButton[actionId]:pressed {
			background-color: transparent;
			border: 2px solid #3b82f6;
		}
	)");
	
	// По умолчанию скрываем контейнер кнопок
	setPanelVisible(true);
}

QToolButton* ActionPanelWidget::createToolButton(const ActionButtonConfig& config) {
	auto* button = new QToolButton(this);

	// Загружаем иконку через сервис текстур (с кэшированием)
	QPixmap pixmap = d->texturesService->getTexture(config.iconName);
	button->setIcon(QIcon(pixmap));

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
	
	// Скрываем/показываем контейнер кнопок и меняем ширину панели
	if (visible) {
		int panelWidth = ItemsStyles::ICON_SIZE + 20;
		setMinimumWidth(panelWidth);
		setMaximumWidth(panelWidth);
		d->buttonsContainer->setVisible(true);
	} else {
		// Оставляем только колонку переключателя (16px)
		setMinimumWidth(16);
		setMaximumWidth(16);
		d->buttonsContainer->setVisible(false);
	}
	
	updateToggleButtonIcon();
}

bool ActionPanelWidget::isPanelVisible() const {
	return d->isVisible;
}

void ActionPanelWidget::togglePanel() {
	setPanelVisible(!d->isVisible);
}

void ActionPanelWidget::onToggleButtonClicked() {
	togglePanel();
}

void ActionPanelWidget::updateToggleButtonIcon() {
	// Стрелка влево (панель раскрыта) или вправо (панель скрыта)
	if (d->isVisible) {
		d->toggleButton->setText("◀");
		d->toggleButton->setToolTip("Свернуть панель");
	} else {
		d->toggleButton->setText("▶");
		d->toggleButton->setToolTip("Развернуть панель");
	}
}
