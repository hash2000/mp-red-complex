#pragma once
#include <QMdiArea>

/// MDI-область с блокировкой всплытия событий прокрутки
/// Предотвращает прокрутку родительского окна, когда скролл достигается границы дочернего окна
class MdiArea : public QMdiArea {
	Q_OBJECT
public:
	explicit MdiArea(QWidget* parent = nullptr);

protected:
	void wheelEvent(QWheelEvent* event) override;
};
