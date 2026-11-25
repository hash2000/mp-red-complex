#include "Libs/BaseWidgets/widgets/background_tasks_indicator.h"
#include "Libs/Base/background_task_registry.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

BackgroundTasksIndicator::BackgroundTasksIndicator(QWidget* parent)
	: QWidget(parent)
	, label(new QLabel(this))
	, timer(new QTimer(this)) {
	setObjectName(QStringLiteral("BackgroundTasksIndicator"));
	label->setObjectName(QStringLiteral("BackgroundTasksIndicatorLabel"));
	label->setContentsMargins(4, 0, 4, 0);

	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(label);

	timer->setInterval(120);
	connect(timer, &QTimer::timeout, this, [this] {
		frame = (frame + 1) % 8;
		updateText();
	});
	connect(&BackgroundTaskRegistry::instance(),
		&BackgroundTaskRegistry::tasksChanged, this, [this] {
			refresh();
		});

	hide();
	refresh();
}

void BackgroundTasksIndicator::refresh() {
	const int count = BackgroundTaskRegistry::instance().taskCount();
	if (count <= 0) {
		timer->stop();
		hide();
		return;
	}

	if (!timer->isActive()) {
		timer->start();
	}
	show();
	updateText();
}

void BackgroundTasksIndicator::updateText() {
	static const QStringList frames = {
		QStringLiteral("⠋"), QStringLiteral("⠙"), QStringLiteral("⠹"),
		QStringLiteral("⠸"), QStringLiteral("⠼"), QStringLiteral("⠴"),
		QStringLiteral("⠦"), QStringLiteral("⠧")
	};
	const QStringList labels = BackgroundTaskRegistry::instance().labels();
	if (labels.isEmpty()) {
		hide();
		return;
	}

	const QString spinner = frames.at(frame % frames.size());
	if (labels.size() == 1) {
		label->setText(QStringLiteral("%1  %2").arg(spinner, labels.first()));
	}
	else {
		label->setText(QStringLiteral("%1  %2 задач").arg(spinner)
			.arg(labels.size()));
	}
	label->setToolTip(labels.join(QLatin1Char('\n')));
}
