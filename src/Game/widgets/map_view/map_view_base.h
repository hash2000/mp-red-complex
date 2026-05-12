#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <memory>
#include <optional>

class TileRenderer;
class TextureAtlas;
class TilesSelectorService;
class MapService;
class ShadersService;
class Camera;

// Базовый класс для отображения карты
class MapViewBase : public QOpenGLWidget {
	Q_OBJECT
public:
	explicit MapViewBase(ShadersService* shadersService, QWidget* parent = nullptr);
	~MapViewBase() override;

	// Управление камерой
	void resetCamera();

signals:
	void initializeContext();
	void tileClicked(std::optional<QPoint> point);
	void tileHovered(std::optional<QPoint> point);
	void beginFrame();

protected:
	// OpenGL методы
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	// Обработка ввода
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

	// Инициализация тайловой системы
	void initializeTileSystem();
	void initializeShaders();

	TileRenderer* tileRenderer() const;

	// Камера
	Camera& camera();
	const Camera& camera() const;
	void setDefaultCamera();

	void setZLevel(float value);

private:
	void setupViewport();

	class Private;
	std::unique_ptr<Private> d;
};
