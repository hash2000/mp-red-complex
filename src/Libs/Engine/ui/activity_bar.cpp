#include "Libs/Engine/ui/activity_bar.h"
#include "Libs/Engine/ui/glyph_tool_button.h"

#include <QEnterEvent>
#include <QHash>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QStyle>

namespace {
constexpr int kExpandedWidth = 48;
constexpr int kCollapsedWidth = 6;
constexpr int kPinTabWidth = 14;
constexpr int kPinTabHeight = 28;
}

struct ActivityBar::Private {
	QWidget* content = nullptr;
	QVBoxLayout* topLayout = nullptr;
	QVBoxLayout* bottomLayout = nullptr;
	QHash<QString, QToolButton*> buttons;
	QToolButton* pinButton = nullptr;
	bool autoHide = false;
	bool expanded = true;
	QTimer collapseTimer;

	void setExpanded(ActivityBar* q, bool e) {
		const bool changed = expanded != e;
		expanded = e;
		content->setVisible(e);
		q->setFixedWidth(e ? kExpandedWidth : kCollapsedWidth);
		q->setProperty("collapsed", !e);
		q->setToolTip(e ? QString() : ActivityBar::tr("Наведите, чтобы показать панель кнопок"));
		q->style()->unpolish(q);
		q->style()->polish(q);
		q->update();
		if (changed) {
			emit q->expandedChanged(e);
		}
	}
};

ActivityBar::ActivityBar(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>()) {
	setObjectName(QStringLiteral("ActivityBar"));
	setAttribute(Qt::WA_StyledBackground);
	setAttribute(Qt::WA_Hover);

	auto* outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->setSpacing(0);

	d->content = new QWidget(this);
	auto* cl = new QVBoxLayout(d->content);
	cl->setContentsMargins(0, 0, 0, 0);
	cl->setSpacing(0);
	d->pinButton = new QToolButton(this);
	d->pinButton->setObjectName(QStringLiteral("pinButton"));
	d->pinButton->setFixedSize(kPinTabWidth, kPinTabHeight);
	d->pinButton->setAutoRaise(true);
	d->pinButton->setArrowType(Qt::RightArrow);
	d->pinButton->setToolTip(tr("Свернуть"));

	auto* pinLayout = new QVBoxLayout;
	pinLayout->setContentsMargins(0, 0, 0, 0);
	pinLayout->addStretch(1);
	pinLayout->addWidget(d->pinButton);
	pinLayout->addStretch(1);

	d->topLayout = new QVBoxLayout;
	d->topLayout->setSpacing(0);
	d->bottomLayout = new QVBoxLayout;
	d->bottomLayout->setSpacing(0);
	cl->addLayout(d->topLayout);
	cl->addStretch(1);
	cl->addLayout(d->bottomLayout);
	outer->addWidget(d->content, 1);
	outer->addLayout(pinLayout);

	d->collapseTimer.setSingleShot(true);
	d->collapseTimer.setInterval(400);
	connect(&d->collapseTimer, &QTimer::timeout, this, [this] {
		d->setExpanded(this, false);
	});

	connect(d->pinButton, &QToolButton::clicked, this, [this]() {
		setAutoHide(!d->autoHide);
	});

	d->setExpanded(this, true);
}

ActivityBar::~ActivityBar() = default;

void ActivityBar::addButton(const QString& id, const QString& glyph, const QString& toolTip,
	Position pos, bool checkable) {
	auto* b = new GlyphToolButton(glyph, toolTip, d->content, "ActivityButton");
	b->setCheckable(checkable);
	b->setFixedWidth(kExpandedWidth);
	connect(b, &QToolButton::clicked, this, [this, id] {
		emit buttonClicked(id);
	});

	(pos == Position::Top ? d->topLayout : d->bottomLayout)->addWidget(b);
	d->buttons.insert(id, b);
}

void ActivityBar::removeButton(const QString& id) {
	if (auto* button = d->buttons.take(id))
		button->deleteLater();
}

void ActivityBar::setActive(const QString& id) {
	for (auto it = d->buttons.cbegin(); it != d->buttons.cend(); it++)
		if (it.value()->isCheckable())
			it.value()->setChecked(it.key() == id);
}

void ActivityBar::setPinVisible(bool visible) {
	d->pinButton->setVisible(visible);
	if (!visible) {
		// Без кнопки сворачивания панель должна оставаться развёрнутой.
		setAutoHide(false);
	}
}

bool ActivityBar::isPinVisible() const {
	return !d->pinButton->isHidden();
}

void ActivityBar::setAutoHide(bool on) {
	d->autoHide = on;
	d->collapseTimer.stop();
	d->setExpanded(this, !on);

	d->pinButton->setArrowType(on ? Qt::LeftArrow : Qt::RightArrow);
	d->pinButton->setToolTip(on ? tr("Закрепить") : tr("Свернуть"));
	emit pinToggled(on);
}

bool ActivityBar::autoHide() const {
	return d->autoHide;
}

void ActivityBar::enterEvent(QEnterEvent* e) {
	if (d->autoHide) {
		d->collapseTimer.stop();
		if (!d->expanded) {
			d->setExpanded(this, true);
		}
	}
	QWidget::enterEvent(e);
}

void ActivityBar::leaveEvent(QEvent* e) {
	if (d->autoHide)
		d->collapseTimer.start();
	QWidget::leaveEvent(e);
}
