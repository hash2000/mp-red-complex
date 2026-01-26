#include "Launcher/map/map_view.h"
#include <QKeyEvent>
#include <QBrush>
#include <QPropertyAnimation>
#include <QScrollBar>

MapView::MapView(ActionsWidget* actionsWidget, QWidget* parent)
: QGraphicsView(parent)
, _actionsWidget(actionsWidget) {
	_scene = new QGraphicsScene(this);
	setScene(_scene);
	setRenderHint(QPainter::Antialiasing, false);
	setDragMode(QGraphicsView::ScrollHandDrag);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	updateView();
}

void MapView::updateView() {
	if (_directionArrow) {
		_scene->removeItem(_directionArrow);
		delete _directionArrow;
		_directionArrow = nullptr;
	}

	// Определяем видимые чанки
	int playerChunkX = _player.pos.x() / GameMap::CHUNK_SIZE;
	int playerChunkY = _player.pos.y() / GameMap::CHUNK_SIZE;

	std::unordered_set<ChunkKey> visibleChunks;
	for (int cy = playerChunkY - 2; cy <= playerChunkY + 2; ++cy) {
		for (int cx = playerChunkX - 2; cx <= playerChunkX + 2; ++cx) {
			ChunkKey key{ cx, cy };
			visibleChunks.insert(key);

			if (_renderedChunks.find(key) == _renderedChunks.end()) {
				// Чанк ещё не отрисован — генерируем и добавляем
				_map.generateChunk(cx, cy);
				auto& tiles = _map.getChunkTiles(cx, cy); // нужно добавить метод!
				auto chunkItem = new ChunkItem(cx, cy, tiles);
				_scene->addItem(chunkItem);
				_renderedChunks[key] = chunkItem;
			}
		}
	}

	// Удаляем невидимые чанки (опционально, для экономии памяти)
	auto it = _renderedChunks.begin();
	while (it != _renderedChunks.end()) {
		if (visibleChunks.count(it->first) == 0) {
			_scene->removeItem(it->second);
			delete it->second;
			it = _renderedChunks.erase(it);
		}
		else {
			++it;
		}
	}

	drawPlayer();
	drawDirectionArrow();
	updateDirectionTileInfo();
}

void MapView::drawChunk(int chunkX, int chunkY) {
	for (int y = 0; y < GameMap::CHUNK_SIZE; ++y) {
		for (int x = 0; x < GameMap::CHUNK_SIZE; ++x) {
			int wx = chunkX * GameMap::CHUNK_SIZE + x;
			int wy = chunkY * GameMap::CHUNK_SIZE + y;
			Tile& t = _map.getOrCreateTile(wx, wy);

			auto rect = _scene->addRect(
				wx * TILE_SIZE, wy * TILE_SIZE,
				TILE_SIZE, TILE_SIZE,
				QPen(Qt::NoPen), QBrush(t.color())
			);
			rect->setZValue(0);
		}
	}
}

void MapView::drawPlayer() {
	if (_playerItem) {
		_scene->removeItem(_playerItem);
		delete _playerItem;
		_playerItem = nullptr;
	}

	QRectF rect(
		_player.pos.x() * TILE_SIZE + 1,
		_player.pos.y() * TILE_SIZE + 1,
		TILE_SIZE - 2,
		TILE_SIZE - 2
	);

	_playerItem = _scene->addRect(rect, QPen(Qt::cyan, 3), QBrush(QColor("#FF4444")));
	_playerItem->setZValue(10);
}

void MapView::setPlayerPosition(const QPoint& pos) {
	_player.pos = pos;
	updateView();
}

void MapView::keyPressEvent(QKeyEvent* event) {
	int newX = _player.pos.x(), newY = _player.pos.y();
	bool moved = false;

	//switch (event->key()) {
	//case Qt::Key_Left:  newX--; moved = true; break;
	//case Qt::Key_Right: newX++; moved = true; break;
	//case Qt::Key_Up:    newY--; moved = true; break;
	//case Qt::Key_Down:  newY++; moved = true; break;
	//default: QGraphicsView::keyPressEvent(event); return;
	//}

	if (event->key() == Qt::Key_Left) { _player.currentDirection = { -1, 0 }; }
	else if (event->key() == Qt::Key_Right) { _player.currentDirection = { 1, 0 }; }
	else if (event->key() == Qt::Key_Up) { _player.currentDirection = { 0, -1 }; }
	else if (event->key() == Qt::Key_Down) { _player.currentDirection = { 0, 1 }; }
	else {
		QGraphicsView::keyPressEvent(event);
		return;
	}

	updateView();

	//// Проверим, можно ли идти
	//Tile& target = _map.getOrCreateTile(newX, newY);
	//if (target.isWalkable()) {
	//	_player.move(newX - _player.pos.x(), newY - _player.pos.y());
	//	updateView();
	//}
}

void MapView::drawDirectionArrow() {
	if (_directionArrow) {
		_scene->removeItem(_directionArrow);
		delete _directionArrow;
		_directionArrow = nullptr;
	}

	if (_player.currentDirection == QPoint(0, 0)) return;

	// Позиция над игроком
	int arrowX = (_player.pos.x() + _player.currentDirection.x()) * TILE_SIZE;
	int arrowY = (_player.pos.y() + _player.currentDirection.y()) * TILE_SIZE;

	// Простая стрелка (треугольник)
	QPolygonF poly;
	if (_player.currentDirection == QPoint(1, 0)) { // right
		poly << QPointF(arrowX + TILE_SIZE, arrowY + TILE_SIZE / 2)
			<< QPointF(arrowX + TILE_SIZE + 8, arrowY)
			<< QPointF(arrowX + TILE_SIZE + 8, arrowY + TILE_SIZE);
	}
	else if (_player.currentDirection == QPoint(-1, 0)) { // left
		poly << QPointF(arrowX, arrowY + TILE_SIZE / 2)
			<< QPointF(arrowX - 8, arrowY)
			<< QPointF(arrowX - 8, arrowY + TILE_SIZE);
	}
	else if (_player.currentDirection == QPoint(0, -1)) { // up
		poly << QPointF(arrowX + TILE_SIZE / 2, arrowY)
			<< QPointF(arrowX, arrowY - 8)
			<< QPointF(arrowX + TILE_SIZE, arrowY - 8);
	}
	else if (_player.currentDirection == QPoint(0, 1)) { // down
		poly << QPointF(arrowX + TILE_SIZE / 2, arrowY + TILE_SIZE)
			<< QPointF(arrowX, arrowY + TILE_SIZE + 8)
			<< QPointF(arrowX + TILE_SIZE, arrowY + TILE_SIZE + 8);
	}

	_directionArrow = _scene->addPolygon(poly, QPen(Qt::white), QBrush(Qt::yellow));
	_directionArrow->setZValue(2);
}

void MapView::updateDirectionTileInfo() {
	if (_player.currentDirection == QPoint(0, 0)) {
		// Передаём пустой тайл или специальный "нет направления"
		emit tileInDirectionChanged(Tile{});
		return;
	}

	QPoint targetPos = _player.pos + _player.currentDirection;
	Tile& t = _map.getOrCreateTile(
		targetPos.x(),
		targetPos.y());

	emit tileInDirectionChanged(t);
}

void MapView::performDig() {
	if (_player.currentDirection == QPoint(0, 0)) return;
	QPoint target = _player.pos + _player.currentDirection;
	Tile& t = _map.getOrCreateTile(target.x(), target.y());
	t._destruction += 25.0f;
	if (t._destruction > 100.0f) t._destruction = 100.0f;

	// Обновляем только один тайл в чанке
	int chunkX = target.x() / GameMap::CHUNK_SIZE;
	int chunkY = target.y() / GameMap::CHUNK_SIZE;
	int localX = (target.x() % GameMap::CHUNK_SIZE + GameMap::CHUNK_SIZE) % GameMap::CHUNK_SIZE;
	int localY = (target.y() % GameMap::CHUNK_SIZE + GameMap::CHUNK_SIZE) % GameMap::CHUNK_SIZE;

	ChunkKey key{ chunkX, chunkY };
	if (_renderedChunks.count(key)) {
		_renderedChunks[key]->updateTile(localX, localY, t);
	}

	// Обновляем ActionsWidget
	updateDirectionTileInfo();
}

void MapView::centerOnPlayer()
{
	QPointF playerScenePos(
		_player.pos.x() * TILE_SIZE + TILE_SIZE / 2.0,
		_player.pos.y() * TILE_SIZE + TILE_SIZE / 2.0
	);

	// Но проще: использовать centerOn
	centerOn(playerScenePos);
}

void MapView::centerOnPlayerAnimated() {
	QPointF target = mapToScene(viewport()->rect().center());
	QPointF playerPos(
		_player.pos.x() * TILE_SIZE + TILE_SIZE / 2,
		_player.pos.y() * TILE_SIZE + TILE_SIZE / 2
	);

	// Используем scroll bars для анимации
	auto hBar = horizontalScrollBar();
	auto vBar = verticalScrollBar();

	QPropertyAnimation* animH = new QPropertyAnimation(hBar, "value");
	animH->setEndValue(hBar->value() + (playerPos.x() - target.x()));
	animH->setDuration(300);

	QPropertyAnimation* animV = new QPropertyAnimation(vBar, "value");
	animV->setEndValue(vBar->value() + (playerPos.y() - target.y()));
	animV->setDuration(300);

	animH->start(QAbstractAnimation::DeleteWhenStopped);
	animV->start(QAbstractAnimation::DeleteWhenStopped);
}
