#include "poi.h"

#include <QBrush>
#include <QPen>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QJsonObject>

#include <math.h>

WorldObject::WorldObject(QObject* parent)
    :QObject(parent), accessMutex(QReadWriteLock::Recursive)
{}

QRectF WorldObject::boundRect() const
{
    return QRectF(-radius(), -radius(), 2*radius(), 2*radius());
}

bool WorldObject::collaide(QPointF point, quint16 r)
{
    return pow(point.x() - pos().x(), 2) + pow(point.y()-pos().y(),2) <= pow(radius()+r, 2);
}

WorldObject& WorldObject::setVolume(qreal v)
{
    QWriteLocker lock(&accessMutex);
    _volume = v;
    return *this;
}

WorldObject &WorldObject::setCapacity(qreal capacity)
{
    _capacity = (capacity < 0)?PI*pow(radius(),2):capacity;
    return *this;
}

WorldObject &WorldObject::setColor(const QColor &color)
{
    _color = color;
    return *this;
}

qreal WorldObject::volume() const
{
    QReadLocker lock(&accessMutex);
    return _volume;
}

qreal WorldObject::capacity() const
{
    return _capacity;
}

QPointF WorldObject::pos() const
{
    return _position;
}

QColor WorldObject::color() const
{
    return _color;
}

WorldObject &WorldObject::setPos(QPointF p)
{
    _position = p;
    return *this;
}

WorldObject &WorldObject::setRadius(qreal radius)
{
    _radius = radius;
    return *this;
}

qreal WorldObject::incVolume(qreal v)
{
    QWriteLocker lock(&accessMutex);
    _volume += v;
    return _volume;
}

qreal WorldObject::decVolume(qreal v) {QWriteLocker lock(&accessMutex); qreal ret = qMin(_volume, v); _volume -= ret; return ret;}

bool WorldObject::tryDecVolume(qreal v)
{
    QWriteLocker lock(&accessMutex);
    if (_volume < v)
        return false;
    _volume -= v;
    return true;
}

qreal WorldObject::sqDistanceTo(QPointF a)
{
    return pow(pos().x()-a.x(), 2) + pow(a.y()-pos().y(), 2);
}

qreal WorldObject::radius() const { return _radius;}

void WorldObject::buildAvatar(QGraphicsScene *scene)
{
    QBrush brush(color());
    QPen pen;
    pen.setWidth(2);

    QGraphicsEllipseItem* gItem = scene->addEllipse(boundRect(), pen, brush);
    gItem->setPos(pos());

    avtr.pEllipse = gItem;
}

PointOfInterestAvatar *WorldObject::avatar() {return &avtr;}

void WorldObject::write(QJsonObject &json) const
{
    if (valid)
    {
        json["id"] = QString::number(reinterpret_cast<quint64>(this), 16);
        json["position"] = QJsonObject({{"x", pos().x()}, {"y", pos().y()}});
        json["radius"] = radius();
        json["volume"] = volume();
        json["capacity"] = capacity();

        json["color_r"] = color().toRgb().red();
        json["color_g"] = color().toRgb().green();
        json["color_b"] = color().toRgb().blue();
        json["color_a"] = color().toRgb().alpha();
    }
}

void WorldObject::read(const QJsonObject & json)
{
    setRadius( json["raidus"].toDouble() );

    if (json.contains("position") && json["position"].isObject())
    {
        setPos({json["position"].toObject()["x"].toDouble(),
               json["position"].toObject()["y"].toDouble()});
    }

}
