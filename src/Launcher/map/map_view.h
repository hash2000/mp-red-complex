#pragma once
#include "Launcher/map/game_map.h"
#include "Launcher/map/player.h"
#include "Launcher/map/chunk_item.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>

class ActionsWidget;
class TaskManager;

class MapView : public QGraphicsView {
  Q_OBJECT
public:
  explicit MapView(TaskManager* taskManager, ActionsWidget* actionsWidget, QWidget* parent = nullptr);

  void updateView();
  void setPlayerPosition(const QPoint& pos);
	void centerOnPlayer();
	void centerOnPlayerAnimated();

signals:
  void tileInDirectionChanged(const Tile& tile);
	void nextPerformDig(const Tile& tile);

public slots:
  void performDig();

protected:
  void keyPressEvent(QKeyEvent* event) override;

private:
  void drawChunk(int chunkX, int chunkY);
  void drawPlayer();
  void drawDirectionArrow();
  void updateDirectionTileInfo();

private:
  static constexpr int TILE_SIZE = 16;

  QGraphicsScene* _scene;
  GameMap _map;
  Player _player;
  QGraphicsPolygonItem* _directionArrow{ nullptr };
  QGraphicsRectItem* _playerItem{ nullptr };
  ActionsWidget* _actionsWidget;
	TaskManager* _taskManager;
  std::unordered_map<ChunkKey, ChunkItem*> _renderedChunks;
};
