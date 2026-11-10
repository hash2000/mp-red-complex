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
}

struct ActivityBar::Private {
	QWidget* content = nullptr;
	QVBoxLayout* topLayout = nullptr;
	QVBoxLayout* bottomLayout = nullptr;
	QHash<QString, QToolButton*> buttons;
	bool autoHide = false;
	bool expanded = true;
	QTimer collapseTimer;

	void setExpanded(ActivityBar* q, bool e) {
		expanded = e;
		content->setVisible(e);
		q->setFixedWidth(e ? kExpandedWidth : kCollapsedWidth);
		q->setProperty("collapsed", !e);
		q->setToolTip(e ? QString() : ActivityBar::tr("Наведите, чтобы показать панель кнопок"));
		q->style()->unpolish(q);
		q->style()->polish(q);
		q->update();
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
	d->topLayout = new QVBoxLayout;
	d->topLayout->setSpacing(0);
	d->bottomLayout = new QVBoxLayout;
	d->bottomLayout->setSpacing(0);
	cl->addLayout(d->topLayout);
	cl->addStretch(1);
	cl->addLayout(d->bottomLayout);
	outer->addWidget(d->content);

	d->collapseTimer.setSingleShot(true);
	d->collapseTimer.setInterval(400);
	connect(&d->collapseTimer, &QTimer::timeout, this, [this] {
		d->setExpanded(this, false);
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

void ActivityBar::setActive(const QString& id) {
	for (auto it = d->buttons.cbegin(); it != d->buttons.cend(); it++)
		if (it.value()->isCheckable())
			it.value()->setChecked(it.key() == id);
}

void ActivityBar::setAutoHide(bool on) {
	d->autoHide = on;
	d->collapseTimer.stop();
	d->setExpanded(this, !on);
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
