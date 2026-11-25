#pragma once
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

/// Глобальный реестр фоновых задач приложения.
///
/// Задачи регистрируются по строковому id: редактор/модуль начинает задачу,
/// обновляет подпись и завершает её. UI статус-бара слушает tasksChanged()
/// и показывает индикатор. Реестр не зависит от конкретных модулей.
class BackgroundTaskRegistry : public QObject {
	Q_OBJECT
public:
	static BackgroundTaskRegistry& instance();

	void beginTask(const QString& id, const QString& label);
	void updateTask(const QString& id, const QString& label);
	void endTask(const QString& id);

	int taskCount() const;
	QStringList labels() const;

signals:
	void tasksChanged();

private:
	explicit BackgroundTaskRegistry(QObject* parent = nullptr);

	QHash<QString, QString> tasks;
	QStringList order;
};
