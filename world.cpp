#include "world.h"
#include "agent.h"
#include <QMutex>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QtConcurrent>

Agent* World::generateNewAgent(QPointF position)
{
    Agent* agent = new Agent(this);
    agent->setPos(position);
    connect (agent, &Agent::newCommunication, this, &World::onNewCommunication);

    emit agentCreated(agent);

    QMutexLocker lock(&agentListAccess);
/*
    agents.begin()->append(agent);
    if (agents.begin()->count() == 50)
        agents.append(QVector<Agent*>());
*/
    agents.append(agent);

    return agent;
}

void World::onStart()
{
    stopRequested = false;
    for (int i =0; i<AGENTS_COUNT; i++)
    {
        bool ok = true;
        QPointF pos;
        do
        {
            pos = randomWorldCoord(10);
            foreach (PointOfInterest* resPo, pResources)
                if (resPo->collaide(pos, 10))
                {
                    ok = false;
                }
            foreach (PointOfInterest* resPo, pWarehouse)
                if (resPo->collaide(pos, 10))
                {
                    ok = false;
                }

        } while (!ok);

        generateNewAgent( pos );
    }

    for (int i=0;i<3;i++)
        onNewWarehouseRequest();

    for (int i=0;i <5; i++)
        onNewResourceRequest();

    iteration();
}

PointOfInterest* World::generateResource()
{
    PointOfInterest* poi = new PointOfInterest;
    poi->radius = RESOURCE_INITIAL_RADIUS;
    poi->pos = randomWorldCoord(poi->radius);
    poi->setVolume(PI * poi->radius * poi->radius);
    poi->criticalVolume = poi->volume();
    poi->color = QColor("blue");
    return poi;
}

PointOfInterest* World::generateWarehouse()
{
    PointOfInterest* poi = new PointOfInterest;
    poi->radius = WAREHOUSE_INITIAL_RADIUS;
    poi->setVolume(0);
    poi->pos = randomWorldCoord(poi->radius);
    poi->criticalVolume = PI * pow(poi->radius, 2);
    poi->color = QColor("orange");
    return poi;
}

World::World(QObject *parent)
    :QObject(parent), size(WORLD_SIZE)
#if QT_VERSION < 0x051400
    ,agentListAccess(QMutex::Recursive)
#endif
{
    acousticSpace = new AcousticSpace(boundRect().toRect());

    agents.append(QVector<Agent*>());

    connect (this, &World::requestNewAgent, this, &World::generateNewAgent, Qt::QueuedConnection);
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
    QMutexLocker lock(&resourcesAccess);
    foreach(PointOfInterest* poi,pResources)
    {
        if ( poi->collaide(pos, r))
            return poi;
    }
    return nullptr;
}

PointOfInterest* World::warehouseAt(QPointF pos, quint16 r)
{
    foreach(PointOfInterest* poi,pWarehouse)
    {
        if ( poi->collaide(pos, r))
            return poi;
    }
    return nullptr;
}

qreal World::grabResource(PointOfInterest *poi, qreal capacity)
{
    QMutexLocker lock(&resourcesAccess);
    qreal ret = 0;

    if (poi->valid)
    {
        ret = poi->decVolume(capacity);

        if (poi->volume() < capacity)
        {
            emit resourceDepleted(poi);
            poi->valid = false;
            //delete poi;
            onNewResourceRequest();
        }
        else
        {
            poi->radius = sqrt(poi->volume() / PI);
        }
    }
    return ret;
}

qreal World::dropResource(PointOfInterest* poi, qreal volume)
{
    qreal ret = 0;
    poi->incVolume(volume);

    if (poi->volume() > WAREHOUSE_RESOURCES_TO_GENERATE_NEW_AGENTS)
    {
        while (poi->tryDecVolume(NEW_AGENT_RESOURCES_PRICE) )
        {
            emit requestNewAgent(poi->pos + QPointF(poi->radius, 0));
        }
    }

    poi->radius = sqrt(qMax(poi->volume(), poi->criticalVolume) / PI);

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

    foreach (PointOfInterest* poi, pResources)
    {
        if (poi->valid)
            continue;
        pResources.removeOne(poi);
        delete poi;
    }


    commLinesAccess.lock();
    communicatedAgents.clear();
    commLinesAccess.unlock();

    acousticSpace->clear();

    agentListAccess.lock();

    auto agentActions = [this](Agent* agent)
    {
        //foreach(Agent* agent, cluster)
        {
            if (agent->state() == Agent::Dead)
            {
                if (agent->avatar()->valid)
                {
                    agent->avatar()->valid = false;
                    emit agentDied(agent);
                }
                else
                {
                    agents.removeOne(agent);
                    //delete agent;
                }
            }
            else
            {
                agent->move();
                agent->acousticShout(*acousticSpace);
                agent->acousticListen(*acousticSpace);
            }
        }
    };

    QtConcurrent::blockingMap(agents, agentActions);

    agentListAccess.unlock();

//    QtConcurrent::blockingMap(agents, [this](Agent* agent)
//    {
//        //foreach(Agent* agent, cluster)
//          agent->acousticShout(*acousticSpace);
//    });


//    QtConcurrent::blockingMap(agents, [this](Agent* agent)
//    {
//        //foreach(Agent* agent, cluster)
//    agent->acousticListen(*acousticSpace);
//    });

    emit iterationEnd(calcTime.elapsed());
    //usleep(GRANULARITY_US);
}

void World::onNewResourceRequest()
{
    PointOfInterest* resource = generateResource();
    pResources.append( resource );

    emit resourceAppeared (resource);
}

void World::onNewWarehouseRequest()
{
    auto ptr = generateWarehouse();
    pWarehouse.append(ptr);

    emit warehouseAppeared(ptr);
}
