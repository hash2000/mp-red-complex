#include "Libs/BaseWidgets/ribbon/ribbon_button.h"

#include <QHBoxLayout>
#include <QResizeEvent>
#include <QToolButton>

namespace {
constexpr int kMainButtonSize = 30;
constexpr int kPropertiesButtonSize = 13;
constexpr int kPropertiesButtonMargin = 1;
} // namespace

struct RibbonButton::Private {
	RibbonButton* q = nullptr;

	QToolButton* main = nullptr;
	/// Значок «свойства» поверх правого нижнего угла основной кнопки.
	QToolButton* properties = nullptr;

	QString propertiesGroup;

	void updateBadge();
};

void RibbonButton::Private::updateBadge() {
	if (!properties || !q) {
		return;
	}

	properties->move(q->width() - kPropertiesButtonSize - kPropertiesButtonMargin,
		q->height() - kPropertiesButtonSize - kPropertiesButtonMargin);
	properties->raise();
}

RibbonButton::RibbonButton(const QString& label, const QString& tooltip, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>()) {
	d->q = this;
	setObjectName(QStringLiteral("RibbonButton"));
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	setCursor(Qt::PointingHandCursor);

	d->main = new QToolButton(this);
	d->main->setObjectName(QStringLiteral("RibbonButtonMain"));
	d->main->setText(label);
	d->main->setToolTip(tooltip);
	d->main->setToolButtonStyle(Qt::ToolButtonTextOnly);
	d->main->setFocusPolicy(Qt::NoFocus);
	d->main->setCursor(Qt::PointingHandCursor);
	d->main->setMinimumSize(kMainButtonSize, kMainButtonSize);

	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(d->main);

	// Значок свойств не входит в layout: он рисуется поверх угла кнопки.
	d->properties = new QToolButton(this);
	d->properties->setObjectName(QStringLiteral("RibbonButtonProperties"));
	d->properties->setText(QStringLiteral("⚙"));
	d->properties->setToolTip(tr("Свойства"));
	d->properties->setToolButtonStyle(Qt::ToolButtonTextOnly);
	d->properties->setFocusPolicy(Qt::NoFocus);
	d->properties->setCursor(Qt::PointingHandCursor);
	d->properties->setFixedSize(kPropertiesButtonSize, kPropertiesButtonSize);
	d->properties->hide();

	connect(d->main, &QToolButton::clicked, this, &RibbonButton::clicked);
	connect(d->main, &QToolButton::toggled, this, &RibbonButton::toggled);
	connect(d->properties, &QToolButton::clicked, this, [this] {
		if (!d->propertiesGroup.isEmpty()) {
			emit propertiesRequested(d->propertiesGroup);
		}
	});
}

RibbonButton::~RibbonButton() = default;

QString RibbonButton::label() const {
	return d->main ? d->main->text() : QString();
}

void RibbonButton::setLabel(const QString& label) {
	if (!d->main) {
		return;
	}

	d->main->setText(label);
	updateGeometry();
}

void RibbonButton::setToolTip(const QString& tooltip) {
	QWidget::setToolTip(tooltip);
	if (d->main) {
		d->main->setToolTip(tooltip);
	}
}

void RibbonButton::setCheckable(bool checkable) {
	if (d->main) {
		d->main->setCheckable(checkable);
	}
}

bool RibbonButton::isCheckable() const {
	return d->main && d->main->isCheckable();
}

void RibbonButton::setChecked(bool checked) {
	if (d->main) {
		d->main->setChecked(checked);
	}
}

bool RibbonButton::isChecked() const {
	return d->main && d->main->isChecked();
}

QString RibbonButton::propertiesGroup() const {
	return d->propertiesGroup;
}

void RibbonButton::setPropertiesGroup(const QString& groupId) {
	d->propertiesGroup = groupId;

	if (!d->properties) {
		return;
	}

	d->properties->setVisible(!groupId.isEmpty());
	d->properties->setToolTip(groupId.isEmpty()
		? QString()
		: tr("Свойства"));
	d->updateBadge();
}

QToolButton* RibbonButton::mainButton() const {
	return d->main;
}

QToolButton* RibbonButton::propertiesButton() const {
	return d->properties;
}

void RibbonButton::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	d->updateBadge();
}
