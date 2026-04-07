#pragma once
#include <QWidget>
#include <memory>

class ZoomableImageView : public QWidget {
	Q_OBJECT
public:
	explicit ZoomableImageView(QWidget* parent = nullptr);
	~ZoomableImageView() override;

	void setPixmap(const QPixmap& pixmap);
	void resetZoom();

	// Настройки сетки
	void setGridEnabled(bool enabled);
	void setGridSize(int gridSizeX, int gridSizeY);
	void setSelectedTileId(int tileId);

signals:
	void zoomChanged(qreal zoom);
	void tileClicked(int tileId);

protected:
	void paintEvent(QPaintEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
