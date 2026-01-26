#pragma once
#include <QPoint>

class Player {
public:
	QPoint pos{ 0, 0 };
	void move(int dx, int dy) {
		pos += QPoint(dx, dy);
	}
};
