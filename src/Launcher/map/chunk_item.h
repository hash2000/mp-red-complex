#pragma once
#include "Launcher/map/tile.h"
#include <QGraphicsItem>
#include <vector>

class ChunkItem : public QGraphicsItem {
public:
	explicit ChunkItem(int chunkX, int chunkY, const std::vector<Tile>& tiles);

	QRectF boundingRect() const override;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

	void updateTile(int localX, int localY, const Tile& newTile);
	void redrawPixmap(); // перерисовать внутренний pixmap

private:
	int _chunkX;
	int _chunkY;
	std::vector<Tile> _tiles;
	QPixmap _pixmap;
	static constexpr int TILE_SIZE = 16;
	static constexpr int CHUNK_SIZE = 32;
};
