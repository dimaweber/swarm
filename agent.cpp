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
    QPolygonF poly;
    for(qreal angle=0; angle < 2*PI; angle += 2*PI/3)
    {
        poly.append({r*cos(angle + PI/2), r*sin(angle+PI/2)});
    }
    avtr.body = scene->addEllipse(boundRect(), pen());
    avtr.direct = scene->addPolygon(poly, pen(), brush());
    avtr.aura = scene->addEllipse({-shoutRange, -shoutRange, 2*shoutRange, 2*shoutRange});
    avtr.head = scene->addLine(0, r, 0, r+3, pen());

    avtr.grp = scene->createItemGroup({avtr.body, avtr.aura, avtr.direct, avtr.head});

    avtr.aura->hide();
}

Agent::State Agent::state() const
{
    if (ttl == 0)
        return Dead;
    return qFuzzyIsNull(carriedResourceVolume)?Empty:Full;
}

qreal Agent::sqDistanceTo(QPointF a)
{
    return pow(pos().x()-a.x(), 2) + pow(a.y()-position.y(), 2);
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
        position -= {dx,dy};

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

    PointOfInterest* warehousePoi = pWorld->warehouseAt(position, r);
    if (warehousePoi)
    {
        if (state() == Full)
        {
            carriedResourceVolume = pWorld->dropResource(warehousePoi, carriedResourceVolume);

        }
        speed.angle += PI;
        distanceToWarehouse = 0;
        position -= {dx,dy};

        if (sqDistanceTo(warehousePoi->pos) < pow(warehousePoi->radius+radius(), 2))
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
