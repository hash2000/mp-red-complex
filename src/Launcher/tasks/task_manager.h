#pragma once
#include "Launcher/tasks/task.h"
#include <QObject>
#include <vector>
#include <memory>

class TaskManager : public QObject {
	Q_OBJECT
public:
	explicit TaskManager(QObject* parent = nullptr);

	void startMining(const QString& biomeName, int durationMs, std::function<void()> onComplete);
	void startSkillTraining(const QString& skillName, int durationMs, std::function<void()> onComplete);

	void update();

signals:
	void taskAdded(int taskId);
	void taskUpdated(int taskId, float progress);
	void taskCompleted(int taskId);

private:
	std::vector<std::unique_ptr<Task>> _tasks;
	int _nextId = 0;
};
