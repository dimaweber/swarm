#include "poi.h"

#include <QBrush>
#include <QPen>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>


PointOfInterest::PointOfInterest()
    :accessMutex(QReadWriteLock::Recursive)
{}

QRectF PointOfInterest::boundRect() const
{
    return QRectF(-radius, -radius, 2*radius, 2*radius);
}

bool PointOfInterest::collaide(QPointF point, quint16 r)
{
    return pow(point.x() - pos.x(), 2) + pow(point.y()-pos.y(),2) <= pow(radius+r, 2);
}

void PointOfInterest::setVolume(qreal v)
{
    QWriteLocker lock(&accessMutex);
    _volume = v;
}

qreal PointOfInterest::volume() const
{
    QReadLocker lock(&accessMutex);
    return _volume;
}

qreal PointOfInterest::incVolume(qreal v)
{
    QWriteLocker lock(&accessMutex);
    _volume += v;
    return _volume;
}

qreal PointOfInterest::decVolume(qreal v) {QWriteLocker lock(&accessMutex); qreal ret = qMin(_volume, v); _volume -= ret; return ret;}

bool PointOfInterest::tryDecVolume(qreal v)
{
    QWriteLocker lock(&accessMutex);
    if (_volume < v)
        return false;
    _volume -= v;
    return true;
}

void PointOfInterest::buildAvatar(QGraphicsScene *scene)
{
    QBrush brush(color);
    QPen pen;
    pen.setWidth(2);

    QGraphicsEllipseItem* gItem = scene->addEllipse(boundRect(), pen, brush);
    gItem->setPos(pos);

    avtr.pEllipse = gItem;
}
