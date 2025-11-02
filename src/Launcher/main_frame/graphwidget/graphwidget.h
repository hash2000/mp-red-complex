#pragma once
#include <QBasicTimer>
#include <QGraphicsView>

class Node;

//! [0]
class GraphWidget : public QGraphicsView
{
    Q_OBJECT

public:
    GraphWidget(QWidget *parent = nullptr);

    void itemMoved();

public slots:
    void shuffle();
    void zoomIn();
    void zoomOut();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    void drawBackground(QPainter *painter, const QRectF &rect) override;

    void scaleView(qreal scaleFactor);

private:
    QBasicTimer timer;
    Node *centerNode;
};
//! [0]
