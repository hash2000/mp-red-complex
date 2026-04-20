#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <memory>
#include <optional>

class TileRenderer;
class TextureAtlas;
class Tileset;
class TilesService;
class MapService;
class Camera;

// Базовый класс для отображения карты
class MapViewBase : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
	Q_OBJECT
public:
	explicit MapViewBase(QWidget* parent = nullptr);
	~MapViewBase() override;

	// Установка сервисов
	void setTilesService(TilesService* tilesService);
	void setMapService(MapService* mapService);

	// Управление камерой
	void resetCamera();

	// Загрузить тайловую карту
	virtual void loadTilemap();

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

	// Виртуальные методы для переопределения в наследниках
	virtual void onTileClicked(std::optional<QPoint> point);
	virtual void onTileServiceConnected();
	virtual void onTileHovered(std::optional<QPoint> point);
	virtual void onMapServiceConnected();
	virtual void onRightMouseDrag(QPoint delta);
	virtual void onZoom(float zoomFactor);

	// Инициализация тайловой системы
	virtual void initializeTileSystem();

	// Доступ к внутренним компонентам
	TileRenderer* tileRenderer() const;
	TextureAtlas* textureAtlas() const;
	Tileset* tileset() const;
	TilesService* tilesService() const;
	MapService* mapService() const;

	// Камера
	Camera& camera();
	const Camera& camera() const;
	void setDefaultCamera();

private:
	void setupViewport();

	class Private;
	std::unique_ptr<Private> d;
};
