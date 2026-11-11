#include "Libs/Engine/ui/side_host_panel.h"
#include "Libs/Engine/ui/glyph_tool_button.h"

#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

struct SideHostPanel::Private {
	QLabel* title = nullptr;
	QStackedWidget* stack = nullptr;
	QHash<QString, QWidget*> panels;
	QHash<QString, QString> titles;
	QString current;
};

SideHostPanel::SideHostPanel(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>()) {
	setObjectName(QStringLiteral("SideHostPanel"));
	setAttribute(Qt::WA_StyledBackground);
	setMinimumWidth(200);

	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	auto* header = new QWidget(this);
	header->setObjectName(QStringLiteral("PanelHeader"));
	header->setAttribute(Qt::WA_StyledBackground);
	auto* hl = new QHBoxLayout(header);
	hl->setContentsMargins(10, 4, 4, 4);
	hl->setSpacing(2);

	d->title = new QLabel(header);
	d->title->setObjectName(QStringLiteral("PanelTitle"));

	auto* close = new GlyphToolButton(QStringLiteral("✕"), tr("Скрыть панель"), header);

	hl->addWidget(d->title);
	hl->addStretch();
	hl->addWidget(close);

	d->stack = new QStackedWidget(this);

	root->addWidget(header);
	root->addWidget(d->stack, 1);

	connect(close, &QToolButton::clicked, this, &SideHostPanel::closeRequested);
}

SideHostPanel::~SideHostPanel() = default;

void SideHostPanel::addPanel(const QString& id, const QString& title, QWidget* panel) {
	d->panels.insert(id, panel);
	d->titles.insert(id, title.toUpper());
	d->stack->addWidget(panel);
	if (d->current.isEmpty())
		setCurrentPanel(id);
}

void SideHostPanel::setCurrentPanel(const QString& id) {
	auto* p = d->panels.value(id);
	if (!p)
		return;
	d->current = id;
	d->stack->setCurrentWidget(p);
	d->title->setText(d->titles.value(id));
}

QString SideHostPanel::currentPanel() const {
	return d->current;
}
QWidget* SideHostPanel::panel(const QString& id) const {
	return d->panels.value(id);
}
