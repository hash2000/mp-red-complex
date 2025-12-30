#include "Launcher/widgets/side_bar/side_bar.h"
#include <QToolButton>
#include <QPushButton>

SideBar::SideBar(QWidget* parent)
	: QWidget(parent)
	, _layout(new QVBoxLayout(this))
{
	_layout->setSpacing(4);
	_layout->setContentsMargins(8, 32, 8, 8);
	_layout->addStretch();
	setFixedWidth(64);
}

void SideBar::addAction(SideBarAction* action)
{
	auto btn = new QPushButton(this);
	btn->setIcon(action->icon());
	btn->setToolTip(action->description());
	btn->setIconSize(QSize(32, 32));
	btn->setCheckable(true);
	btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	btn->setFixedSize(48, 48);

	connect(btn, &QPushButton::clicked, this, [this, btn, action] {
		uncheckAllButtons();
		btn->setChecked(true);
		emit onAction(action);
		action->onAction();
		});

	_buttons[action->id()] = btn;
	_layout->insertWidget(_layout->count() - 1, btn);
}

void SideBar::setButtonVisible(const QString& id, bool visible) {
	if (auto* btn = _buttons.value(id)) {
		btn->setVisible(visible);
	}
}

void SideBar::setButtonChecked(const QString& id, bool check) {
	if (auto* btn = _buttons.value(id)) {
		uncheckAllButtons();
		btn->setChecked(check);
	}
}

void SideBar::uncheckAllButtons() {
	for (auto btn : _buttons) {
		btn->setChecked(false);
	}
}
