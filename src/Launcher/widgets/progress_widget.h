#pragma once
#include <QWidget>
#include <QMap>
#include <QProgressBar>
#include <QVBoxLayout>

class ProgressWidget : public QWidget {
	Q_OBJECT
public:
	explicit ProgressWidget(QWidget* parent = nullptr);

public slots:
	void onTaskAdded(int taskId);
	void onTaskUpdated(int taskId, float progress);
	void onTaskCompleted(int taskId);

private:
	QVBoxLayout* _layout;
	QMap<int, QProgressBar*> _bars;
};
