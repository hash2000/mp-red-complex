#include "Launcher/widgets/progress_widget.h"


ProgressWidget::ProgressWidget(QWidget* parent) : QWidget(parent) {
	_layout = new QVBoxLayout(this);
	_layout->setAlignment(Qt::AlignTop);
}

void ProgressWidget::onTaskAdded(int taskId) {
	auto bar = new QProgressBar(this);
	bar->setRange(0, 100);
	bar->setValue(0);
	bar->setFormat("Задача #" + QString::number(taskId) + " — 0%");
	_layout->addWidget(bar);
	_bars[taskId] = bar;
}

void ProgressWidget::onTaskUpdated(int taskId, float progress) {
	if (_bars.contains(taskId)) {
		int percent = static_cast<int>(progress * 100);
		_bars[taskId]->setValue(percent);
		_bars[taskId]->setFormat(QString("Задача #%1 — %2%").arg(taskId).arg(percent));
	}
}

void ProgressWidget::onTaskCompleted(int taskId) {
	if (_bars.contains(taskId)) {
		_bars[taskId]->setFormat("✅ Завершено!");
	}
}
