#pragma once
#include "Launcher/map/tile.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>

class ActionsWidget : public QWidget {
	Q_OBJECT
public:
	explicit ActionsWidget(QWidget* parent = nullptr);

public slots:
	void onTileInDirectionChanged(const Tile& tile);

signals:
	void digRequested();
	void centerOnPlayerRequested();

private:
	QLabel* biomeLabel;
	QLabel* resourcesLabel;
	QPushButton* digButton;
	QPushButton* searchPlayerButton;
	Tile currentTile;
};
