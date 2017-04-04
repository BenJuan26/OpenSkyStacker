#include "stackergraphicsview.h"

StackerGraphicsView::StackerGraphicsView(QWidget *parent):QGraphicsView(parent)
{

}

void StackerGraphicsView::wheelEvent(QWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15; // see QWheelEvent documentation
    _numScheduledScalings += numSteps;
    if (_numScheduledScalings * numSteps < 0) // if user moved the wheel in another direction, we reset previously scheduled scalings
        _numScheduledScalings = numSteps;

     QTimeLine *anim = new QTimeLine(350, this);
     anim->setUpdateInterval(20);

     connect(anim, SIGNAL (valueChanged(qreal)), SLOT (scalingTime(qreal)));
     connect(anim, SIGNAL (finished()), SLOT (animFinished()));
     anim->start();
}

void StackerGraphicsView::scalingTime(qreal x)
{
 qreal factor = 1.0 + qreal(_numScheduledScalings) / 300.0;
 scale(factor, factor);
}

void StackerGraphicsView::animFinished()
{
 if (_numScheduledScalings > 0)
 _numScheduledScalings--;
 else
 _numScheduledScalings++;
 sender()->~QObject();
}

void StackerGraphicsView::mousePressEvent(QMouseEvent * event) {

    if (event->button() == Qt::LeftButton)
    {
        qDebug() << "Start Drag";
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
        qDebug() << "End Drag";
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
