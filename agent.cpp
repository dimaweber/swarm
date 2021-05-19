#include "agent.h"
#include <QRandomGenerator>

Agent::Agent(World *world, QPointF initialPosition, QObject *parent)
    :QObject(parent), brushEmpty(QColor("red")), brushFull(QColor("green")), agentState(Empty), pWorld(world)
{
    setInitialSpeed();
    if (initialPosition == QPointF())
        setInitialCoord();
    else
        position = initialPosition;

    //distanceToResource = qrand() % world->worldSize().width();
    //distanceToWarehouse = qrand() % world->worldSize().width();
    capacity = PI * r * r;
}

QRectF Agent::boundRect() const
{
    return {-r, -r, 2*r, 2*r};
}

QBrush Agent::brush() const
{
    return (agentState == Empty)?brushEmpty : brushFull;
}

QPen Agent::pen() const
{
    return (agentState == Empty)?penEmpty : penFull;
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
    return agentState;
}

qreal Agent::sqDistanceTo(QPointF a, QPointF b)
{
    return pow(a.x()-b.x(), 2) + pow(a.y()-b.y(), 2);
}

void Agent::acousticShout(AcousticSpace& space)
{
    AcousticMessage msg;
    msg.disatnceToWarehouse = distanceToWarehouse + shoutRange;
    msg.distanceToResource = distanceToResource+shoutRange;
    msg.sender = this;
    space.shout(msg, pos().toPoint(), shoutRange);
}

void Agent::acousticListen(AcousticSpace &space)
{
    QVector<AcousticMessage>& vector = space.cell(pos());
    foreach(const AcousticMessage& v, vector)
    {
        if (v.disatnceToWarehouse < distanceToWarehouse)
        {
            distanceToWarehouse = v.disatnceToWarehouse;
            if (state() == Agent::Full)
            {
                speed.angle = atan2(v.sender->pos().y() - pos().y(),
                                    v.sender->pos().x() - pos().x());
                emit newCommunication(this, v.sender);
            }
        }

        if (v.distanceToResource < distanceToResource)
        {
            distanceToResource = v.distanceToResource;
            if (state() == Agent::Empty)
            {
                speed.angle = atan2(v.sender->pos().y() - pos().y(),
                                    v.sender->pos().x() - pos().x());
                emit newCommunication(this, v.sender);
            }
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


//    emit movedFrom(position);

    position += {dx, dy};
    distanceToResource += speed.dist;
    distanceToWarehouse += speed.dist;

//    emit movedTo(position);

    PointOfInterest* resourcePoi = pWorld->resourceAt(position, r);
    if (resourcePoi)
    {
        if (agentState == Empty)
        {
            carriedResourceVolume = pWorld->grabResource(resourcePoi, capacity);
            if (!qFuzzyIsNull(carriedResourceVolume))
            {
                agentState = Full;
                emit stateChanged();
            }
        }
        distanceToResource = 0;
        speed.angle += PI;
    }

    if (pWorld->isWarehouseAt(position, r))
    {
        if (agentState == Full)
        {
            carriedResourceVolume = pWorld->dropResource(carriedResourceVolume);
            if (qFuzzyIsNull(carriedResourceVolume))
            {
                agentState = Empty;
                emit stateChanged();
            }
        }
        speed.angle += PI;
        distanceToWarehouse = 0;
    }
}
