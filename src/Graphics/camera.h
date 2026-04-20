#pragma once
#include <QVector3D>
#include <QMatrix4x4>
#include <QPoint>
#include <memory>

class Camera {
public:
	Camera();
	Camera(const QVector3D& center, const QVector3D& eye);
	virtual ~Camera();

	void apply(const QVector3D& center, const QVector3D& eye);

	QMatrix4x4 lookAt(const QVector3D& eye, const QVector3D& center, const QVector3D& up);

	void rotate(float deltaX, float deltaY);

	void update();
	void setupViewport(int w, int h);
	void move(const QPoint& delta);
	void zoom(float delta);

	void moveSpeed(float speed);
	float moveSpeed() const;

	const QMatrix4x4& projection() const;
	const QMatrix4x4 view() const;

	std::optional<QVector3D> raycastToGround(const QPoint& screenPos, int screenWidth, int screenHeight);


private:
	class Private;
	std::unique_ptr<Private> d;
};
