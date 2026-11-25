#include "Libs/Base/background_task_registry.h"

BackgroundTaskRegistry& BackgroundTaskRegistry::instance() {
	static BackgroundTaskRegistry registry;
	return registry;
}

BackgroundTaskRegistry::BackgroundTaskRegistry(QObject* parent)
	: QObject(parent) {
}

void BackgroundTaskRegistry::beginTask(const QString& id, const QString& label) {
	if (id.isEmpty()) {
		return;
	}

	const bool isNew = !tasks.contains(id);
	tasks.insert(id, label);
	if (isNew) {
		order.append(id);
	}
	emit tasksChanged();
}

void BackgroundTaskRegistry::updateTask(const QString& id, const QString& label) {
	if (!tasks.contains(id)) {
		beginTask(id, label);
		return;
	}

	if (tasks.value(id) != label) {
		tasks.insert(id, label);
		emit tasksChanged();
	}
}

void BackgroundTaskRegistry::endTask(const QString& id) {
	if (tasks.remove(id) > 0) {
		order.removeAll(id);
		emit tasksChanged();
	}
}

int BackgroundTaskRegistry::taskCount() const {
	return tasks.size();
}

QStringList BackgroundTaskRegistry::labels() const {
	QStringList result;
	result.reserve(order.size());
	for (const QString& id : order) {
		if (tasks.contains(id)) {
			result.append(tasks.value(id));
		}
	}
	return result;
}
