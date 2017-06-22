#ifndef STACKERGRAPHICSVIEW_H
#define STACKERGRAPHICSVIEW_H

#include <QObject>
#include <QWidget>
#include <QGraphicsView>
#include <QDebug>
#include <QWheelEvent>
#include <QTimeLine>
#include <QScrollBar>
#include <QTouchEvent>

//! Custom QGraphicsView for displaying the preview image.
class StackerGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    StackerGraphicsView(QWidget *parent = 0);
private slots:
    void scalingTime(qreal x);
    void animFinished();
private:
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    int _numScheduledScalings;
    int startX;
    int startY;
};

#endif // STACKERGRAPHICSVIEW_H
