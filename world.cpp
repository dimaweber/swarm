#include "world.h"
#include "agent.h"
#include <QMutex>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QtConcurrent>
#include <QJsonArray>

Agent* World::generateNewAgent(QPointF position)
{
    Agent* agent = new Agent(this);
    agent-> setPos(position)
           .setRadius(DEFAULT_INITIAL_AGENT_RADIUS)
           .setCapacity(PI * DEFAULT_INITIAL_AGENT_RADIUS * DEFAULT_INITIAL_AGENT_RADIUS);
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
    for (int i=0;i<3;i++)
        onNewWarehouseRequest();

    for (int i=0;i <5; i++)
        onNewResourceRequest();

    for (int i =0; i<AGENTS_COUNT; i++)
    {
        bool ok = true;
        QPointF pos;
        do
        {
            ok = true;
            pos = randomWorldCoord(10);
            foreach (WorldObject* resPo, pResources)
                if (resPo->collaide(pos, 10))
                {
                    ok = false;
                }
            foreach (WorldObject* resPo, pWarehouse)
                if (resPo->collaide(pos, 10))
                {
                    ok = false;
                }

        } while (!ok);

        generateNewAgent( pos );
    }


    iteration();
}

void World::read(const QJsonObject &json)
{
    size.setHeight( json["size"].toObject()["height"].toInt(DEFAULT_WORLD_SIZE.height()));
    size.setWidth( json["size"].toObject()["width"].toInt(DEFAULT_WORLD_SIZE.width()));
}

void World::write(QJsonObject &json) const
{
    QJsonObject sz;
    sz["width"] = size.width();
    sz["height"] = size.height();
    json["size"] = sz;

    QJsonArray agentsArray;
    forEachAgent([&agentsArray](const Agent* agent)
    {
        QJsonObject agentJson;
        agent->write(agentJson);
        agentsArray.append(agentJson);
    });
    json["agents"] = agentsArray;

    QJsonArray resourcesArray;
    forEachResource([&resourcesArray](const WorldObject* poi)
    {
        QJsonObject poiJson;
        poi->write(poiJson);
        resourcesArray.append(poiJson);
    });
    json["resources"] = resourcesArray;

    QJsonArray warehousesArray;
    forEachWarehouse([&warehousesArray](const WorldObject* poi)
    {
        QJsonObject poiJson;
        poi->write(poiJson);
        warehousesArray.append(poiJson);
    });
    json["warehouses"] = warehousesArray;
}

WorldObject* World::generateResource()
{
    WorldObject* poi = new WorldObject();
    poi->setRadius (RESOURCE_INITIAL_RADIUS)
        .setPos (randomWorldCoord(poi->radius()))
        .setVolume(PI * pow(poi->radius(), 2))
        .setCapacity(PI * pow(poi->radius(), 2))
        .setColor("blue");
    return poi;
}

WorldObject* World::generateWarehouse()
{
    WorldObject* poi = new WorldObject;
    poi->setRadius (WAREHOUSE_INITIAL_RADIUS)
        .setVolume(0)
        .setPos( randomWorldCoord(poi->radius()))
        .setCapacity( PI * pow(poi->radius(), 2))
        .setColor("orange");
    return poi;
}

World::World(QObject *parent)
    :QObject(parent), size(DEFAULT_WORLD_SIZE)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
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

WorldObject *World::resourceAt(QPointF pos, quint16 r)
{
    QMutexLocker lock(&resourcesAccess);
    foreach(WorldObject* poi,pResources)
    {
        if ( poi->collaide(pos, r))
            return poi;
    }
    return nullptr;
}

WorldObject* World::warehouseAt(QPointF pos, quint16 r)
{
    foreach(WorldObject* poi,pWarehouse)
    {
        if ( poi->collaide(pos, r))
            return poi;
    }
    return nullptr;
}

qreal World::grabResource(WorldObject *poi, qreal capacity)
{
    QMutexLocker lock(&resourcesAccess);
    qreal ret = 0;

    if (poi->isValid())
    {
        ret = poi->decVolume(capacity);

        if (poi->volume() < capacity)
        {
            emit resourceDepleted(poi);
            poi->invalidate();
            //delete poi;
            onNewResourceRequest();
        }
        else
        {
            poi->setRadius (sqrt(poi->volume() / PI));
        }
    }
    return ret;
}

qreal World::dropResource(WorldObject* wo, qreal volume)
{
    qreal ret = 0;
    wo->incVolume(volume);

    if (wo->volume() > WAREHOUSE_RESOURCES_TO_GENERATE_NEW_AGENTS)
    {
        while (wo->tryDecVolume(NEW_AGENT_RESOURCES_PRICE) )
        {
            emit requestNewAgent(wo->pos() + QPointF(wo->radius()+3, 0));
        }
    }

    wo->setRadius (sqrt(qMax(wo->volume(), wo->capacity()) / PI));

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

    foreach (WorldObject* poi, pResources)
    {
        if (poi->isValid())
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
    WorldObject* resource = generateResource();
    pResources.append( resource );

    emit resourceAppeared (resource);
}

void World::onNewWarehouseRequest()
{
    auto ptr = generateWarehouse();
    pWarehouse.append(ptr);

    emit warehouseAppeared(ptr);
}
