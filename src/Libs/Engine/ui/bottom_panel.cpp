#include "Libs/Engine/ui/bottom_panel.h"

#include <QEnterEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

struct BottomPanel::Private {
	QWidget* header = nullptr;
	QLabel* title = nullptr;
	QToolButton* pinButton = nullptr;
	QWidget* tabRow = nullptr;
	QStackedWidget* stack = nullptr;
	QTabBar* tabs = nullptr;
	QStringList ids;
	QStringList titles;
	QList<QPointer<QWidget>> pages;
	bool collapsed = false;
	bool collapsable = true;
	bool closable = true;
	bool autoHide = false;
	QTimer autoHideTimer;
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
	hl->addWidget(d->title);
	hl->addStretch();

	d->stack = new QStackedWidget(this);

	d->tabs = new QTabBar;
	d->tabs->setObjectName(QStringLiteral("BottomTabs"));
	d->tabs->setShape(QTabBar::RoundedSouth);
	d->tabs->setDrawBase(false);
	d->tabs->setExpanding(false);
	d->tabs->setDocumentMode(true);

	// Горизонтальная полоска кнопок панелей: слева сверху — маленькая
	// кнопка автоскрытия/закрепления, как pinButton у ActivityBar.
	d->tabRow = new QWidget(this);
	auto* tabLayout = new QHBoxLayout(d->tabRow);
	tabLayout->setContentsMargins(2, 0, 0, 0);
	tabLayout->setSpacing(2);
	d->pinButton = new QToolButton(d->tabRow);
	d->pinButton->setObjectName(QStringLiteral("bottomPinButton"));
	d->pinButton->setFixedSize(18, 18);
	d->pinButton->setAutoRaise(true);
	d->pinButton->setArrowType(Qt::DownArrow);
	d->pinButton->setToolTip(tr("Автоскрытие панели"));
	tabLayout->addWidget(d->pinButton, 0, Qt::AlignTop | Qt::AlignLeft);
	tabLayout->addWidget(d->tabs, 1);

	root->addWidget(d->header);
	root->addWidget(d->stack, 1);
	root->addWidget(d->tabRow);

	connect(d->tabs, &QTabBar::currentChanged, this, [this](int i) {
		if (i < 0 || i >= d->stack->count())
			return;
		d->stack->setCurrentIndex(i);
		d->title->setText(d->titles.value(i));
	});
	// Клик по вкладке: свёрнуто -> развернуть; клик по активной -> свернуть.
	// Когда сворачивание запрещено, активная вкладка панель не сворачивает.
	connect(d->tabs, &QTabBar::tabBarClicked, this, [this](int i) {
		if (i < 0)
			return;

		if (d->collapsed)
			setCollapsed(false);

		else if (d->collapsable && i == d->tabs->currentIndex())
			setCollapsed(true);

	});

	connect(d->pinButton, &QToolButton::clicked, this, [this] {
		setAutoHide(!d->autoHide);
	});

	d->autoHideTimer.setSingleShot(true);
	d->autoHideTimer.setInterval(400);
	connect(&d->autoHideTimer, &QTimer::timeout, this, [this] {
		if (d->autoHide)
			setCollapsed(true);
	});
}

BottomPanel::~BottomPanel() = default;

void BottomPanel::addPanel(const QString& id, const QString& glyph, const QString& title, QWidget* panel) {
	d->ids << id;
	d->titles << title.toUpper();
	d->pages.append(panel);
	d->stack->addWidget(panel); // сначала в стек,
	d->tabs->addTab(glyph + QLatin1Char(' ') + title); // потом вкладка (currentChanged)
	if (d->collapsed)
		setMaximumHeight(collapsedHeight());
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

	if (d->collapsed)
		setMaximumHeight(collapsedHeight());

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
	// Свёрнутая панель: шапки и содержимого нет, остаётся горизонтальная
	// полоска вкладок с кнопками всех панелей и кнопкой автоскрытия.
	d->header->setVisible(!collapsed);
	d->stack->setVisible(!collapsed);
	setMaximumHeight(collapsed ? collapsedHeight() : QWIDGETSIZE_MAX);
	emit collapsedChanged(collapsed);
}

bool BottomPanel::isCollapsed() const {
	return d->collapsed;
}

void BottomPanel::setCollapsable(bool collapsable) {
	d->collapsable = collapsable;
	d->pinButton->setVisible(collapsable);
	if (!collapsable)
		setAutoHide(false);
}

bool BottomPanel::isCollapsable() const {
	return d->collapsable;
}

void BottomPanel::setClosable(bool closable) {
	d->closable = closable;
}

bool BottomPanel::isClosable() const {
	return d->closable;
}

int BottomPanel::collapsedHeight() const {
	// В свёрнутом состоянии видна только горизонтальная полоска вкладок;
	// запасное значение — если Qt ещё не посчитал sizeHint.
	int rowHeight = d->tabRow->sizeHint().height();
	if (rowHeight <= 0) {
		rowHeight = d->tabs->fontMetrics().height() + 10;
	}

	return rowHeight + 1;
}

void BottomPanel::setAutoHide(bool autoHide) {
	if (d->autoHide == autoHide)
		return;
	d->autoHide = autoHide;
	d->autoHideTimer.stop();

	d->pinButton->setArrowType(autoHide ? Qt::UpArrow : Qt::DownArrow);
	d->pinButton->setToolTip(autoHide ? tr("Закрепить панель") : tr("Автоскрытие панели"));

	setCollapsed(autoHide);
}

bool BottomPanel::autoHide() const {
	return d->autoHide;
}

void BottomPanel::enterEvent(QEnterEvent* e) {
	if (d->autoHide) {
		d->autoHideTimer.stop();
		if (d->collapsed)
			setCollapsed(false);
	}
	QWidget::enterEvent(e);
}

void BottomPanel::leaveEvent(QEvent* e) {
	if (d->autoHide && !d->collapsed) {
		d->autoHideTimer.start();
	}
	QWidget::leaveEvent(e);
}
