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
	QLabel* _biomeLabel;
	QLabel* _resourcesLabel;
	QPushButton* _digButton;
	QPushButton* _searchPlayerButton;
	Tile _currentTile;
};
