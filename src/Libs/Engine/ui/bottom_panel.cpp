#include "Libs/Engine/ui/bottom_panel.h"
#include "Libs/Engine/ui/glyph_tool_button.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>

struct BottomPanel::Private {
	QWidget* header = nullptr;
	QLabel* title = nullptr;
	QToolButton* collapseBtn = nullptr;
	QStackedWidget* stack = nullptr;
	QTabBar* tabs = nullptr;
	QStringList ids;
	QStringList titles;
	QList<QPointer<QWidget>> pages;
	bool collapsed = false;
};

BottomPanel::BottomPanel(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>()) {
	setObjectName(QStringLiteral("BottomPanel"));
	setAttribute(Qt::WA_StyledBackground);

	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	d->header = new QWidget(this);
	d->header->setObjectName(QStringLiteral("PanelHeader"));
	d->header->setAttribute(Qt::WA_StyledBackground);
	auto* hl = new QHBoxLayout(d->header);
	hl->setContentsMargins(10, 3, 4, 3);
	hl->setSpacing(2);
	d->title = new QLabel(d->header);
	d->title->setObjectName(QStringLiteral("PanelTitle"));
	d->collapseBtn = new GlyphToolButton(QStringLiteral("▾"), tr("Свернуть"), d->header);
	auto* close = new GlyphToolButton(QStringLiteral("✕"), tr("Скрыть (Ctrl+J)"), d->header);
	hl->addWidget(d->title);
	hl->addStretch();
	hl->addWidget(d->collapseBtn);
	hl->addWidget(close);

	d->stack = new QStackedWidget(this);

	d->tabs = new QTabBar(this);
	d->tabs->setObjectName(QStringLiteral("BottomTabs"));
	d->tabs->setShape(QTabBar::RoundedSouth);
	d->tabs->setDrawBase(false);
	d->tabs->setExpanding(false);
	d->tabs->setDocumentMode(true);

	root->addWidget(d->header);
	root->addWidget(d->stack, 1);
	root->addWidget(d->tabs);

	connect(d->tabs, &QTabBar::currentChanged, this, [this](int i) {
		if (i < 0 || i >= d->stack->count())
			return;
		d->stack->setCurrentIndex(i);
		d->title->setText(d->titles.value(i));
	});
	// Клик по вкладке: свёрнуто -> развернуть; клик по активной -> свернуть
	connect(d->tabs, &QTabBar::tabBarClicked, this, [this](int i) {
		if (i < 0)
			return;

		if (d->collapsed)
			setCollapsed(false);

		else if (i == d->tabs->currentIndex())
			setCollapsed(true);

	});

	connect(d->collapseBtn, &QToolButton::clicked, this, [this] {
		setCollapsed(!d->collapsed);
	});

	connect(close, &QToolButton::clicked, this, &BottomPanel::closeRequested);
}

BottomPanel::~BottomPanel() = default;

void BottomPanel::addPanel(const QString& id, const QString& glyph, const QString& title, QWidget* panel) {
	d->ids << id;
	d->titles << title.toUpper();
	d->pages.append(panel);
	d->stack->addWidget(panel); // сначала в стек,
	d->tabs->addTab(glyph + QLatin1Char(' ') + title); // потом вкладка (currentChanged)
}

void BottomPanel::setCurrentPanel(const QString& id) {
	const int idx = d->ids.indexOf(id);
	if (idx < 0)
		return;
	d->tabs->setCurrentIndex(idx);
	if (d->collapsed)
		setCollapsed(false);
}

bool BottomPanel::removePanel(const QString& id) {
	const int index = d->ids.indexOf(id);
	if (index < 0)
		return false;

	QPointer<QWidget> page = d->pages.value(index);

	const int currentIndex = d->tabs->currentIndex();
	const bool removedCurrent = (currentIndex == index);
	const bool wasVisible = isVisible();

	d->tabs->blockSignals(true);
	d->stack->blockSignals(true);

	d->tabs->removeTab(index);
	if (page)
		d->stack->removeWidget(page);

	d->ids.removeAt(index);
	d->titles.removeAt(index);
	d->pages.removeAt(index);

	if (!d->ids.isEmpty()) {
		int newIndex = 0;

		if (removedCurrent) {
			newIndex = qMin(index, int(d->ids.size()) - 1);
		}
		else {
			newIndex = (index < currentIndex) ? currentIndex - 1 : currentIndex;
		}

		if (newIndex < 0)
			newIndex = 0;

		d->tabs->setCurrentIndex(newIndex);
		d->stack->setCurrentIndex(newIndex);
		d->title->setText(d->titles.value(newIndex));
	}
	else {
		d->title->clear();
	}

	d->stack->blockSignals(false);
	d->tabs->blockSignals(false);

	if (page) {
		// removeWidget не снимает родителя: иначе страница доживёт до
		// разрушения стека и уничтожится после его владельца.
		page->setParent(nullptr);
		page->hide();
		page->deleteLater();
	}

	if (d->ids.isEmpty() && wasVisible)
		hide();

	return true;
}

QWidget* BottomPanel::panel(const QString& id) const {
	const int idx = d->ids.indexOf(id);
	return idx >= 0 ? d->pages.value(idx).data() : nullptr;
}

void BottomPanel::setCollapsed(bool collapsed) {
	if (d->collapsed == collapsed)
		return;
	d->collapsed = collapsed;
	d->header->setVisible(!collapsed);
	d->stack->setVisible(!collapsed);
	setMaximumHeight(collapsed ? d->tabs->sizeHint().height() + 1 : QWIDGETSIZE_MAX);
	d->collapseBtn->setText(collapsed ? QStringLiteral("▴") : QStringLiteral("▾"));
	emit collapsedChanged(collapsed);
}

bool BottomPanel::isCollapsed() const {
	return d->collapsed;
}
