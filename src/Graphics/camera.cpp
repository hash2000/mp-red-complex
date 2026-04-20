#include "Graphics/camera.h"

class Camera::Private{
public:
	Private(Camera* parent) : q(parent) {
	}

	Camera* q;

	float fov = 45.0f;
	float moveSpeed = 0.01f;
	QVector3D eye = QVector3D(5.0f, 8.0f, 5.0f);
	QVector3D center = QVector3D(0.0f, 0.0f, 0.0f);
	QVector3D up = QVector3D(0.0f, 1.0f, 0.0f);
	QMatrix4x4 projection;
	QMatrix4x4 view;
};

Camera::Camera()
: d(std::make_unique<Private>(this)) {
}

Camera::Camera(const QVector3D& center, const QVector3D& eye)
: d(std::make_unique<Private>(this)) {
	apply(center, eye);
}

Camera::~Camera() = default;

void Camera::apply(const QVector3D& center, const QVector3D& eye) {
	d->center = center;
	d->eye = eye;
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

void Camera::rotate(float deltaX, float deltaY)
{
	// Скорость вращения (можно вынести в настройки, если нужно)
	const float rotationSpeed = 0.005f;
	const float pitchLimit = qDegreesToRadians(85.0f); // Ограничение угла наклона (чтобы не перевернуться)

	// 1. Вычисляем текущий вектор от центра к глазу
	QVector3D offset = d->eye - d->center;
	float distance = offset.length();

	if (distance < 1e-6f) {
		// Защита от деления на ноль
		return;
	}

	// Нормализуем вектор направления
	QVector3D dir = offset / distance;

	// 2. Горизонтальное вращение (Yaw) вокруг оси Up (0, 1, 0)
	// Создаем матрицу вращения вокруг оси Y
	QMatrix4x4 rotYaw;
	rotYaw.rotate(-deltaX * rotationSpeed * 180.0f / 3.14159f, d->up); // Переводим радианы в градусы для QMatrix4x4::rotate

	// Применяем вращение к вектору направления
	QVector3D newDir = rotYaw.mapVector(dir);

	// 3. Вертикальное вращение (Pitch) - изменение высоты
	// Для этого нам нужно найти ось, перпендикулярную и направлению взгляда, и оси Up.
	// Это вектор "Right" в локальной системе камеры.
	QVector3D right = QVector3D::crossProduct(newDir, d->up).normalized();

	// Если newDir почти параллелен up, crossProduct даст ноль. Проверяем.
	if (right.length() > 1e-6f) {
		QMatrix4x4 rotPitch;
		// Ограничиваем угол, чтобы камера не ушла за горизонт
		// Текущий угол можно вычислить через dot product с плоскостью XZ, 
		// но проще просто ограничить смещение deltaY, если оно слишком большое, 
		// или использовать более сложную логику сферических координат.
		// Здесь используем простой подход: вращаем вокруг оси 'right'.

		// Важно: нужно проверить текущий угол наклона, чтобы не превысить лимит.
		float currentPitch = std::asin(QVector3D::dotProduct(newDir, d->up));
		float targetPitch = currentPitch - (deltaY * rotationSpeed);

		// Клампинг угла
		if (targetPitch > pitchLimit) targetPitch = pitchLimit;
		if (targetPitch < -pitchLimit) targetPitch = -pitchLimit;

		// Если мы достигли лимита и пытаемся идти дальше, игнорируем ввод по Y
		if ((targetPitch == pitchLimit && deltaY < 0) ||
			(targetPitch == -pitchLimit && deltaY > 0)) {
			// Угол заблокирован, применяем только Yaw (который уже посчитан выше в newDir)
			// Но newDir уже изменен Yaw, так что просто оставляем его как есть для высоты?
			// Нет, нужно вернуть старую высоту, если движение заблокировано.
			// Проще пересчитать newDir заново с учетом ограничений.

			// Восстанавливаем исходный вектор перед Pitch для корректного расчета, если нужно,
			// но здесь мы просто скорректируем итоговый вектор.

			// Альтернативный простой способ для игр: просто вращаем матрицей и потом нормализуем проекцию на Y.
			// Но самый надежный способ для "изометрии" - явно задать новый угол.

			float sinPitch = std::sin(targetPitch);
			float cosPitch = std::cos(targetPitch);

			// Проекция текущего newDir (после Yaw) на плоскость XZ
			QVector3D horizontalDir = newDir;
			horizontalDir.setY(0.0f);
			horizontalDir.normalize();

			// Формируем новый вектор направления с нужным углом наклона
			newDir = horizontalDir * cosPitch + d->up * sinPitch;
		}
		else {
			// Применяем вращение Pitch вручную через тригонометрию, так надежнее чем матрица для ограничения угла
			float sinPitch = std::sin(targetPitch);
			float cosPitch = std::cos(targetPitch);

			// Проекция направления после Yaw на горизонтальную плоскость
			QVector3D horizontalDir = newDir;
			horizontalDir.setY(0.0f);
			if (horizontalDir.length() > 1e-6f) {
				horizontalDir.normalize();
				newDir = horizontalDir * cosPitch + d->up * sinPitch;
			}
		}
	}

	// 4. Обновляем позицию глаза
	d->eye = d->center + newDir * distance;

	update();
}

void Camera::update()
{
	d->view = lookAt(d->eye, d->center, d->up);
}

void Camera::setupViewport(int w, int h)
{
	float aspect = static_cast<float>(w) / h;
	d->projection.setToIdentity();
	d->projection.perspective(d->fov, aspect, 0.001f, 1000.0f);
	update();
}

void Camera::setFov(float value) {
	d->fov = value;
}

float Camera::fov() const {
	return d->fov;
}

void Camera::move(const QPoint& delta)
{
	// Получаем направление "вперёд" и "вправо" в мировой системе координат
	// На основе текущего взгляда камеры, но проигнорируем Y
	QVector3D viewDir = d->center - d->eye;
	float distance = viewDir.length();
	
	viewDir.setY(0.0f);
	if (viewDir.length() < 1e-6f) {
		viewDir = QVector3D(0.0f, 0.0f, -1.0f); // fallback
	}

	viewDir.normalize();
	QVector3D rightDir = QVector3D::crossProduct(viewDir, QVector3D(0.0f, 1.0f, 0.0f))
		.normalized();

	// Масштабируем скорость в зависимости от расстояния до центра
	// Можно использовать линейную или логарифмическую зависимость
	float scale = qMax(1.0f, distance * 0.1f); // подберите коэффициент (0.1) под себя
	float effectiveSpeed = d->moveSpeed * scale;

	QVector3D panOffset = -rightDir * static_cast<float>(delta.x()) * effectiveSpeed
		+ viewDir * static_cast<float>(delta.y()) * effectiveSpeed;

	// Двигаем и глаз, и центр ОДИНАКОВО
	d->eye += panOffset;
	d->center += panOffset;

	update();
}

void Camera::zoom(float delta)
{
	// Вектор от глаза к центру
	QVector3D viewDir = d->center - d->eye;
	float currentDistance = viewDir.length();

	// Минимальное и максимальное расстояние
	float minDistance = 2.0f;
	float maxDistance = 300.0f;

	float newDistance = currentDistance * delta;
	newDistance = qBound(minDistance, newDistance, maxDistance);

	// Новый глаз = центр - нормализованный вектор * новое расстояние
	viewDir.normalize();
	d->eye = d->center - viewDir * newDistance;

	update();
}

std::optional<QVector3D> Camera::raycastToGround(const QPoint& screenPos, int screenWidth, int screenHeight) {
	float x = (2.0f * screenPos.x()) / screenWidth - 1.0f;
	float y = -(2.0f * screenPos.y()) / screenHeight + 1.0f;

	QVector4D nearPoint(x, y, -1.0f, 1.0f);
	QVector4D farPoint(x, y, 1.0f, 1.0f);

	QMatrix4x4 invViewProj = (d->projection * d->view).inverted();
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
	d->moveSpeed = speed;
}

float Camera::moveSpeed() const
{
	return d->moveSpeed;
}

const QMatrix4x4& Camera::projection() const
{
	return d->projection;
}

const QMatrix4x4 Camera::view() const
{
	return d->view;
}
