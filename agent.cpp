#include "agent.h"
#include <QRandomGenerator>
#include <QJsonObject>

Agent::Agent(World *world, QPointF initialPosition, QObject *parent)
    : WorldObject(parent), colorEmpty(QColor("red")), colorFull(QColor("green")), pWorld(world)
{
    setInitialSpeed();
    if (initialPosition == QPointF())
        setInitialCoord();
    else
        setPos( initialPosition );

    ttl = QRandomGenerator::system()->bounded(6000, 10000);
}

QColor Agent::color() const
{
    return (state() == Empty)?colorEmpty : colorFull;
}


AgentAvatar* Agent::avatar() {return &avtr; }

void Agent::buildAvatar(QGraphicsScene *scene)
{
    QPolygonF poly;
    for(qreal angle=0; angle < 2*PI; angle += 2*PI/3)
    {
        poly.append({radius()*cos(angle + PI/2), radius()*sin(angle+PI/2)});
    }
    avtr.body = scene->addEllipse(boundRect(), color());
    avtr.direct = scene->addPolygon(poly, color(), color());
    avtr.aura = scene->addEllipse({-shoutRange, -shoutRange, 2*shoutRange, 2*shoutRange});
    avtr.head = scene->addLine(0, radius(), 0, radius()+3, color());

    avtr.grp = scene->createItemGroup({avtr.body, avtr.aura, avtr.direct, avtr.head});

    avtr.aura->hide();
}

Agent::State Agent::state() const
{
    if (ttl == 0)
        return Dead;
    return qFuzzyIsNull(volume())?Empty:Full;
}

void Agent::acousticShout(AcousticSpace& space)
{
    AcousticMessage msg;
    msg.minDistanceToResourceSender = this;
    msg.minDistanceToWarehouse = distanceToWarehouse + shoutRange;
    msg.minDistanceToResource = distanceToResource+shoutRange;
    msg.minDistanceToWarehouseSender = this;

    space.shout(msg, pos().toPoint(), (int)shoutRange);
}

void Agent::acousticListen(AcousticSpace &space)
{
    AcousticMessage& v = space.cell(pos());
    v.minDistanceToWarehouseAccess.lock();
    if (v.minDistanceToWarehouse < distanceToWarehouse && v.minDistanceToWarehouseSender)
    {
        distanceToWarehouse = static_cast<quint32>(v.minDistanceToWarehouse);
        if (state() == Agent::Full)
        {
            speed.angle = atan2(v.minDistanceToWarehouseSender->pos().y() - pos().y(),
                                v.minDistanceToWarehouseSender->pos().x() - pos().x());
            emit newCommunication(this, v.minDistanceToWarehouseSender);
        }
    }
    v.minDistanceToWarehouseAccess.unlock();

    if (v.minDistanceToResource < distanceToResource && v.minDistanceToResourceSender)
    {
        distanceToResource = static_cast<quint32>(v.minDistanceToResource);
        if (state() == Agent::Empty)
        {
            speed.angle = atan2(v.minDistanceToResourceSender->pos().y() - pos().y(),
                                v.minDistanceToResourceSender->pos().x() - pos().x());
            emit newCommunication(this, v.minDistanceToResourceSender);
        }
    }
}

void Agent::read(const QJsonObject &json)
{
    WorldObject::read(json);

    if (json.contains("speed") && json["speed"].isObject())
    {
        speed.angle = json["speed"].toObject()["angle"].toDouble();
        speed.dist = json["speed"].toObject()["distance"].toDouble();
    }
}

void Agent::write(QJsonObject &json) const
{
    WorldObject::write(json);
    json["speed"] = QJsonObject({ {"angle", speed.angle}, {"distance", speed.dist}});
    json["shout_range"] = shoutRange;
    json["ttl"] = ttl;
    json["distance_to_resource"] = distanceToResource;
    json["distance_to_warehouse"] = distanceToWarehouse;
}

void Agent::setInitialSpeed()
{
    speed.dist = QRandomGenerator::system()->bounded(1.0) + 2;
    speed.angle = QRandomGenerator::system()->bounded(2 * PI);
}

void Agent::setInitialCoord()
{
    setPos( pWorld->randomWorldCoord(radius()) );
}

void Agent::move()
{
    if (--ttl == 0)
        return;

    bool changeDirection = false;
    qreal dx = speed.dist * cos(speed.angle);
    qreal dy = speed.dist * sin(speed.angle);
    if (pos().x() + dx + radius()> pWorld->maxXcoord()  )
    {
        dx = pWorld->maxXcoord() - (pos().x()  + dx + radius());
        changeDirection = true;
    }
    if (pos().x() + dx - radius() < pWorld->minXcoord() )
    {
        dx = pWorld->minXcoord() - (pos().x() + dx - radius());
        changeDirection = true;
    }
    if (pos().y()+dy + radius()> pWorld->maxYcoord() )
    {
        dy = pWorld->maxYcoord() - (pos().y()  + dy + radius());
        changeDirection = true;
    }
    if (pos().y()+dy - radius()< pWorld->minYcoord() )
    {
        dy = pWorld->minYcoord() - (pos().y() + dy - radius());
        changeDirection = true;
    }

    if (changeDirection)
    {
        speed.angle += QRandomGenerator::system()->bounded(PI);
        while (speed.angle < -PI)
            speed.angle += 2 * PI;
        while (speed.angle > PI)
            speed.angle -= 2 * PI;
    }

    QPointF delta(dx, dy);
    setPos( pos() + delta);
    distanceToResource += speed.dist;
    distanceToWarehouse += speed.dist;

    WorldObject* resourcePoi = pWorld->resourceAt(pos(), radius());
    if (resourcePoi)
    {
        if (state() == Empty)
        {
            setVolume( pWorld->grabResource(resourcePoi, capacity()));
        }
        distanceToResource = 0;
        speed.angle += PI;
        setPos( pos()- delta);

        /*
        if (sqDistanceTo(resourcePoi->pos) < pow(resourcePoi->radius+radius(), 2))
        {
            // we're inside resource, move to edge
            qreal s = sqrt(pow(resourcePoi->pos.x()-pos().x(), 2)+pow(resourcePoi->pos.y()-pos().y(), 2)) / (pos().x()-resourcePoi->pos.x());
            qreal c = sqrt(pow(resourcePoi->pos.x()-pos().x(), 2)+pow(resourcePoi->pos.y()-pos().y(), 2)) / (pos().y()-resourcePoi->pos.y());
            qreal x = resourcePoi->radius * c + resourcePoi->pos.x();
            qreal y = resourcePoi->radius * s + resourcePoi->pos.y();
            position = {x, y};
        }
        */
    }

    WorldObject* warehousePoi = pWorld->warehouseAt(pos(), radius());
    if (warehousePoi)
    {
        if (state() == Full)
        {
            setVolume( pWorld->dropResource(warehousePoi, volume()));

        }
        speed.angle += PI;
        distanceToWarehouse = 0;
        setPos( pos() - delta);

        if (sqDistanceTo(warehousePoi->pos()) < pow(warehousePoi->radius()+radius(), 2))
        {
            // we're inside resource, move to edge

//            qreal s = sqrt(pow(warehousePoi->pos.x()-pos().x(), 2)+pow(warehousePoi->pos.y()-pos().y(), 2)) / (pos().x()-warehousePoi->pos.x());
//            qreal c = sqrt(pow(warehousePoi->pos.x()-pos().x(), 2)+pow(warehousePoi->pos.y()-pos().y(), 2)) / (pos().y()-warehousePoi->pos.y());
//            qreal x = (warehousePoi->radius + radius()) * c + warehousePoi->pos.x();
//            qreal y = (warehousePoi->radius + radius()) * s + warehousePoi->pos.y();
//            position = {x, y};

        }
    }
}
