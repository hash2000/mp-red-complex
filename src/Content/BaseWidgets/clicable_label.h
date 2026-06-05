#pragma once
#include <QLabel>

class ClickableLabel : public QLabel {
	Q_OBJECT
public:
	explicit ClickableLabel(QWidget* parent = nullptr) : QLabel(parent) {
		setCursor(Qt::OpenHandCursor);
	}

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
};
