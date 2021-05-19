#include "world.h"
#include "agent.h"
#include <QMutex>
#include <QElapsedTimer>
#include <QRandomGenerator>

bool operator < (const QPoint& a, const QPoint& b)
{
    return a.x() < b.x() || a.x() == b.x() && a.y() < b.y();
}

Agent* World::generateNewAgent()
{
    QMutexLocker lock(&agentListAccess);

    Agent* agent = new Agent(this);
    agents.append(agent);
    agent->setPos(randomWorldCoord(agent->radius()));
    connect (agent, &Agent::newCommunication, this, &World::onNewCommunication);

    emit agentCreated(agent);
    return agent;
}

void World::onStart()
{
    stopRequested = false;
    for (int i =0; i<AGENTS_COUNT; i++)
    {
        generateNewAgent();
    }

    onNewWarehouseRequest();
    onNewResourceRequest();
    onNewResourceRequest();

    iteration();
}

PointOfInterest* World::generateResource()
{
    PointOfInterest* poi = new PointOfInterest;
    poi->radius = RESOURCE_INITIAL_RADIUS;
    poi->pos = randomWorldCoord(poi->radius);
    poi->volume = PI * poi->radius * poi->radius;
    poi->criticalVolume = poi->volume;
    poi->color = QColor("blue");
    return poi;
}

PointOfInterest* World::generateWarehouse()
{
    PointOfInterest* poi = new PointOfInterest;
    poi->radius = WAREHOUSE_INITIAL_RADIUS;
    poi->volume = 0;
    poi->pos = randomWorldCoord(poi->radius);
    poi->criticalVolume = PI * pow(poi->radius, 2);
    poi->color = QColor("orange");
    return poi;
}

World::World(QObject *parent)
    :QObject(parent), size(WORLD_SIZE)
{
    acousticSpace = new AcousticSpace(boundRect().toRect());
}

void World::stop()
{
    stopRequested = true;
}

QSizeF World::worldSize() const
{
    return size;
}

QRectF World::boundRect() const
{
    QPoint vertex(- size.width() / 2, - size.height()/2);
    QRect rect( vertex, size);
    return rect;
}

QPointF World::randomWorldCoord(qreal margin) const
{
    QPointF point (QRandomGenerator::system()->bounded((qint32)(minXcoord() + margin), (qint32)(maxXcoord() - margin)),
                   QRandomGenerator::system()->bounded((qint32)(minYcoord() + margin), (qint32)(maxYcoord() - margin)));
    return point;
}

qreal World::minXcoord() const
{
    return boundRect().left();
}

qreal World::maxXcoord() const
{
    return boundRect().right();
}

qreal World::minYcoord() const
{
    return boundRect().top();
}

qreal World::maxYcoord() const
{
    return boundRect().bottom();
}

PointOfInterest *World::resourceAt(QPointF pos, quint16 r)
{
    foreach(PointOfInterest* poi,pResources)
    {
        if ( poi->collaide(pos, r))
            return poi;
    }
    return nullptr;
}

bool World::isWarehouseAt(QPointF pos, quint16 r)
{
    return pWarehouse?pWarehouse->collaide(pos, r):false;
}

qreal World::grabResource(PointOfInterest *poi, qreal capacity)
{
    QMutexLocker lock(&resourcesAccess);

    qreal ret = 0;
    ret = qMin(poi->volume, capacity);

    poi->volume -= ret;

    if (poi->volume < capacity)
    {
        emit resourceDepleted(poi->pAvatar);
        pResources.removeOne(poi);
        poi->valid = false;
        //delete poi;
        onNewResourceRequest();
    }
    else
    {
        poi->radius = sqrt(poi->volume / PI);
    }

    return ret;
}

qreal World::dropResource(qreal volume)
{
    qreal ret = 0;
    pWarehouse->volume += volume;

    if (pWarehouse->volume > WAREHOUSE_RESOURCES_TO_GENERATE_NEW_AGENTS)
    {
        while (pWarehouse->volume > NEW_AGENT_RESOURCES_PRICE)
        {
            Agent* agent = generateNewAgent();
            agent->setPos(pWarehouse->pos + QPointF(pWarehouse->radius, 0));
            pWarehouse->volume -= NEW_AGENT_RESOURCES_PRICE;
        }
    }

    pWarehouse->radius = sqrt(qMax(pWarehouse->volume, pWarehouse->criticalVolume) / PI);

    return ret;
}

void World::onNewCommunication(Agent* a, Agent* b)
{
    commLinesAccess.lock();
    communicatedAgents.append(QPair<Agent*, Agent*>(a, b));
    commLinesAccess.unlock();
}

void World::iteration()
{
    QElapsedTimer calcTime;
    calcTime.start();
    emit iterationStart();
    commLinesAccess.lock();
    communicatedAgents.clear();
    commLinesAccess.unlock();

    agentListAccess.lock();
    foreach (Agent* agent, agents)
        agent->move();

    agentListAccess.unlock();

    acousticSpace->clear();

    foreach(Agent* agent, agents)
        agent->acousticShout(*acousticSpace);

    foreach(Agent* agent, agents)
        agent->acousticListen(*acousticSpace);

    emit iterationEnd(calcTime.elapsed());
    //usleep(GRANULARITY_US);
}

void World::onNewResourceRequest()
{
    PointOfInterest* resource = generateResource();
    emit resourceAppeared (resource);
    pResources.append( resource );
}

void World::onNewWarehouseRequest()
{
    pWarehouse = generateWarehouse();
    emit warehouseAppeared(pWarehouse);
}
