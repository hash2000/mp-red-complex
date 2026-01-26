#pragma once
#include <QString>
#include <QElapsedTimer>
#include <functional>

class Task {
public:
	enum class Type {
		Mining,
		SkillTraining,
		Crafting
	};

	Task(Type type, QString name, int durationMs, std::function<void()> onComplete);

	bool isCompleted() const;
	float getProgress() const;
	QString getName() const;
	Type getType() const;

	void update();

private:
	std::function<void()> onComplete;
	Type _type;
	QString _name;
	int _totalDuration;
	QElapsedTimer _timer;
	float _progress = 0.0f;
};
