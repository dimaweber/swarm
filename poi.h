#ifndef POI_H
#define POI_H

#include <QBrush>
#include <QPen>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QThread>

#include <QReadWriteLock>

struct PointOfInterestAvatar
{
    QGraphicsEllipseItem* pEllipse = nullptr;

    void setBrush(QBrush b)
    {
        pEllipse->setBrush(b);
    }

    QBrush brush() const { return pEllipse->brush(); }

    void setPos(QPointF p)
    {
        pEllipse->setPos(p);
    }

    void setAlpha(qreal a)
    {
        QBrush b = brush();
        QColor c = b.color();
        c.setAlpha(a);
        b.setColor(c);
        setBrush(b);
    }

    void setRect(QRectF r)
    {
        pEllipse->setRect(r);
    }

    void setBorderWidth(int w)
    {
        QPen pen = pEllipse->pen();
        pen.setWidth(w);
        pEllipse->setPen(pen);
    }

    void destroy()
    {
        QGraphicsScene* scene = pEllipse->scene();
        if (scene->thread() != QThread::currentThread())
            throw "wrong thread";
        scene->removeItem(pEllipse);
    }
};

struct PointOfInterest
{
    QPointF pos;
    qreal radius;
    qreal criticalVolume;
    QColor color;
    bool valid = true;
    mutable QReadWriteLock accessMutex;

    PointOfInterest();

    QRectF boundRect() const;

    bool collaide(QPointF point, quint16 r);

    void setVolume(qreal v);
    qreal volume() const;
    qreal incVolume(qreal v);
    qreal decVolume(qreal v);
    bool tryDecVolume(qreal v);

    void buildAvatar(QGraphicsScene* scene);
    PointOfInterestAvatar* avatar() {return &avtr;}

private:
    PointOfInterestAvatar avtr;
    qreal _volume;
};

#endif // POI_H
