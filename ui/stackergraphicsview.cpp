#include "stackergraphicsview.h"

StackerGraphicsView::StackerGraphicsView(QWidget *parent):QGraphicsView(parent)
{

}

void StackerGraphicsView::wheelEvent(QWheelEvent *event)
{
    QPoint movement = event->pixelDelta();
    int threshold = 8;
    int movementPace = movement.y();
    if (_numScheduledScalings > 0 && movementPace < 0) {
        _numScheduledScalings = 0;
    } else if  (_numScheduledScalings < 0 && movementPace > 0) {
        _numScheduledScalings = 0;
    } else {
        _numScheduledScalings += movementPace;
    }

    if (_numScheduledScalings > threshold) {
        _numScheduledScalings = threshold;
    } else if (_numScheduledScalings < -threshold) {
        _numScheduledScalings = -threshold;
    }

    QTimeLine *anim = new QTimeLine(350, this);
    anim->setUpdateInterval(100);

    connect(anim, SIGNAL (valueChanged(qreal)), SLOT (scalingTime(qreal)));
    connect(anim, SIGNAL (finished()), SLOT (animFinished()));
    anim->start();
}

void StackerGraphicsView::scalingTime(qreal x)
{
    qreal factor = 1.0 + qreal(_numScheduledScalings) / 300.0;
    scale(factor, factor);

    Q_UNUSED(x);
}

void StackerGraphicsView::animFinished()
{
    if (_numScheduledScalings > 0) {
        _numScheduledScalings--;
    } else if (_numScheduledScalings < 0) {
        _numScheduledScalings++;
    }
    sender()->~QObject();
}

void StackerGraphicsView::mousePressEvent(QMouseEvent * event) {

    if (event->button() == Qt::LeftButton)
    {
        this->setDragMode(QGraphicsView::ScrollHandDrag);
        startX = event->x();
        startY = event->y();
        this->setInteractive(true);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void StackerGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
    {
        this->setDragMode(QGraphicsView::NoDrag);
        this->setInteractive(false);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void StackerGraphicsView::mouseMoveEvent(QMouseEvent *event) {
    if (this->dragMode() == QGraphicsView::ScrollHandDrag) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - startX));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - startY));
        startX = event->x();
        startY = event->y();
        event->accept();
        return;
    }
}
