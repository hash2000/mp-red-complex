#include "Launcher/tasks/task_manager.h"
#include <algorithm>

TaskManager::TaskManager(QObject* parent)
: QObject(parent) {
}

void TaskManager::startMining(const QString& biomeName, int durationMs, std::function<void()> onComplete) {
	_tasks.push_back(std::make_unique<Task>(
		Task::Type::Mining,
		"Добыча: " + biomeName,
		durationMs,
		[this, id = _nextId, onComplete]() {
			emit taskCompleted(id);
			onComplete();
		}
	));
	emit taskAdded(_nextId++);
}

void TaskManager::startSkillTraining(const QString& skillName, int durationMs, std::function<void()> onComplete) {
	_tasks.push_back(std::make_unique<Task>(
		Task::Type::SkillTraining,
		"Навык: " + skillName,
		durationMs,
		[this, id = _nextId, onComplete]() {
			emit taskCompleted(id);
			onComplete();
		}
	));
	emit taskAdded(_nextId++);
}

void TaskManager::update() {
	// Обновляем все задачи
	for (size_t i = 0; i < _tasks.size(); ++i) {
		if (!_tasks[i]->isCompleted()) {
			_tasks[i]->update();
			emit taskUpdated(static_cast<int>(i), _tasks[i]->getProgress());
		}
	}

	// Удаляем завершённые задачи
	_tasks.erase(
		std::remove_if(_tasks.begin(), _tasks.end(),
			[](const auto& t) {
				return t->isCompleted();
			}),
		_tasks.end()
	);
}
