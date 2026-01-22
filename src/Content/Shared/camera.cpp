#include "Content/Shared/camera.h"

Camera::Camera(const QVector3D& center, const QVector3D& eye)
: _center(center)
, _eye(eye) {
}

QMatrix4x4 Camera::lookAt(const QVector3D& eye, const QVector3D& center, const QVector3D& up)
{
	QMatrix4x4 view;
	QVector3D f = (center - eye).normalized();
	QVector3D s = QVector3D::crossProduct(f, up).normalized();
	QVector3D u = QVector3D::crossProduct(s, f);

	view(0, 0) = s.x();  view(0, 1) = s.y();  view(0, 2) = s.z();
	view(1, 0) = u.x();  view(1, 1) = u.y();  view(1, 2) = u.z();
	view(2, 0) = -f.x(); view(2, 1) = -f.y(); view(2, 2) = -f.z();

	view(0, 3) = -QVector3D::dotProduct(s, eye);
	view(1, 3) = -QVector3D::dotProduct(u, eye);
	view(2, 3) = QVector3D::dotProduct(f, eye);

	return view;
}

void Camera::update()
{
	_view = lookAt(_eye, _center, _up);
}

void Camera::setupViewport(int w, int h)
{
	float aspect = static_cast<float>(w) / h;
	_projection.setToIdentity();
	_projection.perspective(45.0f, aspect, 0.1f, 100.0f);
	update();
}

void Camera::move(const QPoint& delta)
{
	// Получаем направление "вперёд" и "вправо" в мировой системе координат
	// На основе текущего взгляда камеры, но проигнорируем Y
	QVector3D viewDir = _center - _eye;
	float distance = viewDir.length();
	
	viewDir.setY(0.0f);
	if (viewDir.length() < 1e-6f) {
		viewDir = QVector3D(0.0f, 0.0f, -1.0f); // fallback
	}

	viewDir.normalize();
	QVector3D rightDir = QVector3D::crossProduct(viewDir, QVector3D(0.0f, 1.0f, 0.0f)).normalized();

	// Масштабируем скорость в зависимости от расстояния до центра
	// Можно использовать линейную или логарифмическую зависимость
	float scale = qMax(1.0f, distance * 0.1f); // подберите коэффициент (0.1) под себя
	float effectiveSpeed = _moveSpeed * scale;

	QVector3D panOffset = -rightDir * static_cast<float>(delta.x()) * effectiveSpeed
		+ viewDir * static_cast<float>(delta.y()) * effectiveSpeed;

	// Двигаем и глаз, и центр ОДИНАКОВО
	_eye += panOffset;
	_center += panOffset;

	update();
}

void Camera::zoom(float delta)
{
	// Вектор от глаза к центру
	QVector3D viewDir = _center - _eye;
	float currentDistance = viewDir.length();

	// Минимальное и максимальное расстояние
	float minDistance = 2.0f;
	float maxDistance = 300.0f;

	float newDistance = currentDistance * delta;
	newDistance = qBound(minDistance, newDistance, maxDistance);

	// Новый глаз = центр - нормализованный вектор * новое расстояние
	viewDir.normalize();
	_eye = _center - viewDir * newDistance;

	update();
}

std::optional<QVector3D> Camera::raycastToGround(const QPoint& screenPos, int screenWidth, int screenHeight) {
	float x = (2.0f * screenPos.x()) / screenWidth - 1.0f;
	float y = -(2.0f * screenPos.y()) / screenHeight + 1.0f;

	QVector4D nearPoint(x, y, -1.0f, 1.0f);
	QVector4D farPoint(x, y, 1.0f, 1.0f);

	QMatrix4x4 invViewProj = (_projection * _view).inverted();
	QVector4D rayNear4 = invViewProj * nearPoint;
	QVector4D rayFar4 = invViewProj * farPoint;

	// Перспективное деление
	if (std::abs(rayNear4.w()) < 1e-6f || std::abs(rayFar4.w()) < 1e-6f) {
		return std::nullopt;
	}

	QVector3D rayNear(rayNear4.x() / rayNear4.w(),
		rayNear4.y() / rayNear4.w(),
		rayNear4.z() / rayNear4.w());
	QVector3D rayFar(rayFar4.x() / rayFar4.w(),
		rayFar4.y() / rayFar4.w(),
		rayFar4.z() / rayFar4.w());

	QVector3D rayDir = (rayFar - rayNear).normalized();
	QVector3D origin = rayNear;

	// Пересечение с Y = 0
	if (std::abs(rayDir.y()) < 1e-6f) {
		return std::nullopt;
	}

	float t = -origin.y() / rayDir.y();
	if (t < 0) {
		return std::nullopt;
	}

	QVector3D hit = origin + t * rayDir;
	return hit;
}

void Camera::moveSpeed(float speed)
{
	_moveSpeed = speed;
}

float Camera::moveSpeed() const
{
	return _moveSpeed;
}

const QMatrix4x4& Camera::projection() const
{
	return _projection;
}

const QMatrix4x4 Camera::view() const
{
	return _view;
}
