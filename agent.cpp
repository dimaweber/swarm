#include "agent.h"
#include <QRandomGenerator>

Agent::Agent(World *world, QPointF initialPosition, QObject *parent)
    :QObject(parent), brushEmpty(QColor("red")), brushFull(QColor("green")), penEmpty("red"), penFull("green"), pWorld(world)
{
    setInitialSpeed();
    if (initialPosition == QPointF())
        setInitialCoord();
    else
        position = initialPosition;

    capacity = PI * r * r;

    ttl = QRandomGenerator::system()->bounded(6000, 10000);
}

QRectF Agent::boundRect() const
{
    return {-r, -r, 2*r, 2*r};
}

QBrush Agent::brush() const
{
    return (state() == Empty)?brushEmpty : brushFull;
}

QPen Agent::pen() const
{
    return (state() == Empty)?penEmpty : penFull;
}

QPointF Agent::pos() const
{
    return position;
}

AgentAvatar* Agent::avatar() {return &avtr; }

void Agent::buildAvatar(QGraphicsScene *scene)
{
    avtr.body = scene->addEllipse(boundRect(), pen(), brush());
    avtr.aura = scene->addEllipse({-shoutRange, -shoutRange, 2*shoutRange, 2*shoutRange});
    avtr.grp = scene->createItemGroup({avtr.body, avtr.aura});

    avtr.aura->hide();
}

Agent::State Agent::state() const
{
    if (ttl == 0)
        return Dead;
    return qFuzzyIsNull(carriedResourceVolume)?Empty:Full;
}

qreal Agent::sqDistanceTo(QPointF a, QPointF b)
{
    return pow(a.x()-b.x(), 2) + pow(a.y()-b.y(), 2);
}

void Agent::acousticShout(AcousticSpace& space)
{
    AcousticMessage msg;
    msg.minDistanceToResourceSender = this;
    msg.minDistanceToWarehouse = distanceToWarehouse + shoutRange;
    msg.minDistanceToResource = distanceToResource+shoutRange;
    msg.minDistanceToWarehouseSender = this;

    space.shout(msg, pos().toPoint(), shoutRange);
}

void Agent::acousticListen(AcousticSpace &space)
{
    AcousticMessage& v = space.cell(pos());
    v.minDistanceToWarehouseAccess.lockForRead();
    if (v.minDistanceToWarehouse < distanceToWarehouse && v.minDistanceToWarehouseSender)
    {
        distanceToWarehouse = v.minDistanceToWarehouse;
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
        distanceToResource = v.minDistanceToResource;
        if (state() == Agent::Empty)
        {
            speed.angle = atan2(v.minDistanceToResourceSender->pos().y() - pos().y(),
                                v.minDistanceToResourceSender->pos().x() - pos().x());
            emit newCommunication(this, v.minDistanceToResourceSender);
        }
    }
}

void Agent::setInitialSpeed()
{
    speed.dist = QRandomGenerator::system()->bounded(1.0) + 2;
    speed.angle = QRandomGenerator::system()->bounded(2 * PI);
}

void Agent::setInitialCoord()
{
    position = {0, 0}; pWorld->randomWorldCoord(r);
}

void Agent::move()
{
    if (--ttl == 0)
        return;

    bool changeDirection = false;
    qreal dx = speed.dist * cos(speed.angle);
    qreal dy = speed.dist * sin(speed.angle);
    if (position.x() + dx + r> pWorld->maxXcoord()  )
    {
        dx = pWorld->maxXcoord() - (position.x()  + dx + r);
        changeDirection = true;
    }
    if (position.x() + dx - r < pWorld->minXcoord() )
    {
        dx = pWorld->minXcoord() - (position.x() + dx - r);
        changeDirection = true;
    }
    if (position.y()+dy + r> pWorld->maxYcoord() )
    {
        dy = pWorld->maxYcoord() - (position.y()  + dy + r);
        changeDirection = true;
    }
    if (position.y()+dy - r< pWorld->minYcoord() )
    {
        dy = pWorld->minYcoord() - (position.y() + dy - r);
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

    position += {dx, dy};
    distanceToResource += speed.dist;
    distanceToWarehouse += speed.dist;

    PointOfInterest* resourcePoi = pWorld->resourceAt(position, r);
    if (resourcePoi)
    {
        if (state() == Empty)
        {
            carriedResourceVolume = pWorld->grabResource(resourcePoi, capacity);
        }
        distanceToResource = 0;
        speed.angle += PI;
    }

    PointOfInterest* warehousePoi = pWorld->warehouseAt(position, r);
    if (warehousePoi)
    {
        if (state() == Full)
        {
            carriedResourceVolume = pWorld->dropResource(warehousePoi, carriedResourceVolume);

        }
        speed.angle += PI;
        distanceToWarehouse = 0;
    }
}
