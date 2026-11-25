#pragma once
#include <QWidget>

class QLabel;
class QTimer;

/// Индикатор фоновых задач для статус-бара приложения.
///
/// Показывает вращающийся значок и подпись/количество активных задач.
/// Подписывается на глобальный BackgroundTaskRegistry.
class BackgroundTasksIndicator : public QWidget {
	Q_OBJECT
public:
	explicit BackgroundTasksIndicator(QWidget* parent = nullptr);

private:
	void refresh();
	void updateText();

	QLabel* label = nullptr;
	QTimer* timer = nullptr;
	int frame = 0;
};
