#pragma once
#include <QPoint>

class Player {
public:
	void move(int dx, int dy);

public:
	QPoint pos{ 0, 0 };
	QPoint currentDirection{ 0, 0 };
};
