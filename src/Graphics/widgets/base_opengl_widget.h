#pragma once
#include <QOpenGLWidget>
#include <memory>

class Camera;

class BaseOpenGLWidget : public QOpenGLWidget {
public:
	explicit BaseOpenGLWidget(QWidget* parent = nullptr);
	~BaseOpenGLWidget() override;

public:
	void resetCamera();

signals:
	void initializeContext();
	void beginFrame();
	void tileClicked(std::optional<QPoint> point);
	void tileHovered(std::optional<QPoint> point);

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

	Camera& camera();
	const Camera& camera() const;
	void setDefaultCamera();

	void setZLevel(float value);
	float zLevel() const;

private:
	void setupViewport();

private:
	class Private;
	std::unique_ptr<Private> d;
};
