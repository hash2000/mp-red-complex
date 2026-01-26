#include "Launcher/map/chunk_item.h"
#include <QPainter>

ChunkItem::ChunkItem(int chunkX, int chunkY, const std::vector<Tile>& tiles)
: _chunkX(chunkX)
, _chunkY(chunkY)
, _tiles(tiles) {
	setPos(chunkX * CHUNK_SIZE * TILE_SIZE, chunkY * CHUNK_SIZE * TILE_SIZE);
	redrawPixmap();
}

QRectF ChunkItem::boundingRect() const {
	return QRectF(0, 0, CHUNK_SIZE * TILE_SIZE, CHUNK_SIZE * TILE_SIZE);
}

void ChunkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
	painter->drawPixmap(0, 0, _pixmap);
}

void ChunkItem::redrawPixmap() {
	_pixmap = QPixmap(CHUNK_SIZE * TILE_SIZE, CHUNK_SIZE * TILE_SIZE);
	_pixmap.fill(Qt::transparent);
	QPainter p(&_pixmap);
	for (int y = 0; y < CHUNK_SIZE; ++y) {
		for (int x = 0; x < CHUNK_SIZE; ++x) {
			const Tile& t = _tiles[y * CHUNK_SIZE + x];
			p.fillRect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, t.color());
		}
	}
	p.end();
}

void ChunkItem::updateTile(int localX, int localY, const Tile& newTile) {
	if (localX >= 0 && localX < CHUNK_SIZE && localY >= 0 && localY < CHUNK_SIZE) {
		_tiles[localY * CHUNK_SIZE + localX] = newTile;

		QPainter p(&_pixmap);
		p.fillRect(localX * TILE_SIZE, localY * TILE_SIZE, TILE_SIZE, TILE_SIZE, newTile.color());
		p.end();
		update();
	}
}
