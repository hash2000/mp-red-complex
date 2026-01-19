#pragma once
#include <QVector3D>
#include <QMatrix4x4>

class Camera {
public:
	Camera(const QVector3D& center, const QVector3D& eye);

	QMatrix4x4 lookAt(const QVector3D& eye, const QVector3D& center, const QVector3D& up);
	void update();
	void setupViewport(int w, int h);
	void move(const QPoint& delta);
	void zoom(float delta);

	void moveSpeed(float speed);
	float moveSpeed() const;

	const QMatrix4x4& projection() const;
	const QMatrix4x4 view() const;

private:
	float _moveSpeed = 0.01f;
	QVector3D _eye = QVector3D(5.0f, 8.0f, 5.0f);
	QVector3D _center = QVector3D(0.0f, 0.0f, 0.0f);
	QVector3D _up = QVector3D(0.0f, 1.0f, 0.0f);
	QMatrix4x4 _projection;
	QMatrix4x4 _view;
};
