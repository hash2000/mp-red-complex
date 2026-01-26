#include "Launcher/tasks/task.h"

Task::Task(Type type, QString name, int durationMs, std::function<void()> onComplete)
: _type(type)
, _name(std::move(name))
, _totalDuration(durationMs)
, onComplete(std::move(onComplete)) {
	_timer.start();
}

void Task::update() {
	if (isCompleted()) {
		return;
	}

	qint64 elapsed = _timer.elapsed();
	_progress = static_cast<float>(elapsed) / static_cast<float>(_totalDuration);
	if (_progress >= 1.0f) {
		_progress = 1.0f;
		if (onComplete) {
			onComplete();
		}
	}
}

bool Task::isCompleted() const {
	return _progress >= 1.0f;
}

float Task::getProgress() const {
	return _progress;
}

QString Task::getName() const {
	return _name;
}

Task::Type Task::getType() const {
	return _type;
}
