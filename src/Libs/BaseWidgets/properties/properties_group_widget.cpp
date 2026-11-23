#include "Libs/BaseWidgets/properties/properties_group_widget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

struct PropertiesGroupWidget::Private {
	QPushButton* header = nullptr;
	/// Подпись группы внутри кнопки-заголовка: выравнивается по левому краю
	/// независимо от стиля темы.
	QLabel* titleLabel = nullptr;
	QWidget* contentContainer = nullptr;
	QVBoxLayout* contentLayout = nullptr;

	std::function<QWidget*()> factory;
	QWidget* content = nullptr;

	QString groupId;
	QString title;

	bool expanded = false;

	/// Создаёт ленивое содержимое, если группа раскрыта и оно ещё не создано.
	void ensureContent();
};

void PropertiesGroupWidget::Private::ensureContent() {
	if (content || !factory) {
		return;
	}

	auto create = factory;
	factory = nullptr;

	QWidget* widget = create ? create() : nullptr;
	if (!widget) {
		return;
	}

	content = widget;
	widget->setParent(contentContainer);
	contentLayout->addWidget(widget);
	// Видимостью управляет контейнер группы: содержимое не прячем насовсем,
	// иначе после раскрытия группы оно останется скрытым.
	widget->setVisible(true);
}

void PropertiesGroupWidget::updateHeader() {
	if (!d->titleLabel) {
		return;
	}

	const QString arrow = d->expanded ? QStringLiteral("▾") : QStringLiteral("▸");
	d->titleLabel->setText(QStringLiteral("%1  %2").arg(arrow, d->title));
	if (d->header) {
		d->header->setToolTip(d->expanded
			? tr("Свернуть группу")
			: tr("Развернуть группу"));
	}
}

PropertiesGroupWidget::PropertiesGroupWidget(const QString& title, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>()) {
	d->title = title;
	setObjectName(QStringLiteral("PropertiesGroup"));

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	d->header = new QPushButton(this);
	d->header->setObjectName(QStringLiteral("PropertiesGroupHeader"));
	d->header->setCheckable(true);
	d->header->setFlat(true);
	d->header->setCursor(Qt::PointingHandCursor);

	// Подпись — отдельная метка внутри кнопки: так заголовок всегда стоит
	// у левого края, даже если тема не задаёт text-align для кнопки.
	auto* headerLayout = new QHBoxLayout(d->header);
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setSpacing(0);
	d->titleLabel = new QLabel(d->header);
	d->titleLabel->setObjectName(QStringLiteral("PropertiesGroupTitle"));
	d->titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	d->titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
	headerLayout->addWidget(d->titleLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);

	layout->addWidget(d->header);

	d->contentContainer = new QWidget(this);
	d->contentContainer->setObjectName(QStringLiteral("PropertiesGroupContent"));
	d->contentLayout = new QVBoxLayout(d->contentContainer);
	d->contentLayout->setContentsMargins(6, 4, 2, 6);
	d->contentLayout->setSpacing(4);
	d->contentContainer->hide();
	layout->addWidget(d->contentContainer);

	connect(d->header, &QPushButton::toggled, this, [this](bool checked) {
		setExpanded(checked);
	});

	updateHeader();
}

PropertiesGroupWidget::~PropertiesGroupWidget() = default;

QString PropertiesGroupWidget::groupId() const {
	return d->groupId;
}

void PropertiesGroupWidget::setGroupId(const QString& id) {
	d->groupId = id;
}

QString PropertiesGroupWidget::title() const {
	return d->title;
}

void PropertiesGroupWidget::setTitle(const QString& title) {
	d->title = title;
	updateHeader();
}

void PropertiesGroupWidget::setContent(QWidget* content) {
	if (d->content == content) {
		return;
	}

	if (d->content) {
		d->contentLayout->removeWidget(d->content);
		d->content->deleteLater();
	}

	d->content = content;
	d->factory = nullptr;

	if (!content) {
		return;
	}

	content->setParent(d->contentContainer);
	d->contentLayout->addWidget(content);
	// Содержимое не скрываем: когда группа раскроется, контейнер покажет его.
	content->setVisible(true);
}

void PropertiesGroupWidget::setContentFactory(std::function<QWidget*()> factory) {
	d->factory = std::move(factory);

	if (d->expanded) {
		d->ensureContent();
	}
}

QWidget* PropertiesGroupWidget::content() const {
	return d->content;
}

bool PropertiesGroupWidget::isExpanded() const {
	return d->expanded;
}

void PropertiesGroupWidget::setExpanded(bool expanded) {
	const bool changed = d->expanded != expanded;
	d->expanded = expanded;

	if (d->header) {
		// Программное изменение состояния не должно снова вызывать setExpanded().
		const QSignalBlocker blocker(d->header);
		d->header->setChecked(expanded);
	}

	if (expanded) {
		d->ensureContent();
	}

	if (d->contentContainer) {
		d->contentContainer->setVisible(expanded);
	}

	// Содержимое могло быть создано, пока группа была свёрнута: при
	// раскрытии показываем его вместе с контейнером.
	if (d->content) {
		d->content->setVisible(expanded);
	}

	updateHeader();

	if (changed) {
		emit expandedChanged(expanded);
	}
}
