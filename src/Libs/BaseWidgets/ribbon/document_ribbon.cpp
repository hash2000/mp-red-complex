#include "Libs/BaseWidgets/ribbon/document_ribbon.h"
#include "Libs/BaseWidgets/ribbon/ribbon_button.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kToggleSize = 20;
} // namespace

struct DocumentRibbon::Private {
	QWidget* groups = nullptr;
	QToolButton* toggle = nullptr;

	bool collapsed = false;

	void updateToggle();
};

void DocumentRibbon::Private::updateToggle() {
	if (!toggle) {
		return;
	}

	toggle->setText(collapsed ? QStringLiteral("⌄") : QStringLiteral("⌃"));
	toggle->setToolTip(collapsed
		? DocumentRibbon::tr("Показать панель")
		: DocumentRibbon::tr("Скрыть панель"));
}

DocumentRibbon::DocumentRibbon(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>()) {
	setObjectName(QStringLiteral("DocumentRibbon"));
	setAttribute(Qt::WA_StyledBackground);

	auto* row = new QHBoxLayout(this);
	row->setContentsMargins(6, 4, 4, 4);
	row->setSpacing(6);

	d->groups = new QWidget(this);
	d->groups->setObjectName(QStringLiteral("RibbonGroups"));
	auto* groupsLayout = new QHBoxLayout(d->groups);
	groupsLayout->setContentsMargins(0, 0, 0, 0);
	groupsLayout->setSpacing(6);

	d->groups->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	row->addWidget(d->groups, 0, Qt::AlignLeft | Qt::AlignTop);
	row->addStretch(1);

	// Небольшая кнопка в правом углу ленты: свернуть или показать её.
	d->toggle = new QToolButton(this);
	d->toggle->setObjectName(QStringLiteral("RibbonToggle"));
	d->toggle->setFixedSize(kToggleSize, kToggleSize);
	d->toggle->setCursor(Qt::PointingHandCursor);
	d->toggle->setFocusPolicy(Qt::NoFocus);
	connect(d->toggle, &QToolButton::clicked, this, [this] {
		setCollapsed(!d->collapsed);
	});
	row->addWidget(d->toggle, 0, Qt::AlignRight | Qt::AlignTop);

	d->updateToggle();
}

DocumentRibbon::~DocumentRibbon() = default;

QHBoxLayout* DocumentRibbon::addGroup(const QString& title) {
	if (!d->groups) {
		return nullptr;
	}

	auto* group = new QFrame(d->groups);
	group->setObjectName(QStringLiteral("RibbonGroup"));
	group->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

	auto* layout = new QVBoxLayout(group);
	layout->setContentsMargins(6, 4, 6, 2);
	layout->setSpacing(2);

	auto* content = new QHBoxLayout;
	content->setContentsMargins(0, 0, 0, 0);
	content->setSpacing(4);

	auto* caption = new QLabel(title.toUpper(), group);
	caption->setObjectName(QStringLiteral("RibbonGroupCaption"));
	caption->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

	layout->addLayout(content, 1);
	layout->addWidget(caption, 0, Qt::AlignHCenter);

	if (auto* groupsLayout = qobject_cast<QHBoxLayout*>(d->groups->layout())) {
		groupsLayout->addWidget(group);
	}

	return content;
}

RibbonButton* DocumentRibbon::addButton(QHBoxLayout* group, const QString& label,
	const QString& tooltip, const QString& propertiesGroup) {
	if (!group || !group->parentWidget()) {
		return nullptr;
	}

	auto* button = new RibbonButton(label, tooltip, group->parentWidget());
	button->setPropertiesGroup(propertiesGroup);
	group->addWidget(button);

	connect(button, &RibbonButton::propertiesRequested, this,
		&DocumentRibbon::propertiesRequested);

	return button;
}

void DocumentRibbon::setCollapsed(bool collapsed) {
	const bool changed = d->collapsed != collapsed;
	d->collapsed = collapsed;

	if (d->groups) {
		d->groups->setVisible(!collapsed);
	}

	d->updateToggle();

	if (changed) {
		emit collapsedChanged(collapsed);
	}
}

bool DocumentRibbon::isCollapsed() const {
	return d->collapsed;
}
