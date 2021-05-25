#ifndef WORLD_H
#define WORLD_H

#include "poi.h"

#include <QSize>
#include <QPointF>
#include <QGraphicsEllipseItem>
#include <QBrush>
#include <QPen>
#include <QDataStream>
#include <QVector>
#include <QThread>
#include <QMutex>
#if QT_VERSION >= 0x051400
#include <QRecursiveMutex>
#endif
#include <QReadWriteLock>
#include <QMap>

#include <math.h>

const qreal PI = 3.1415926;

const quint16 AGENTS_COUNT = 500;
const quint16 GRANULARITY_US = 1000;
const quint32 WAREHOUSE_RESOURCES_TO_GENERATE_NEW_AGENTS = 1000;
const quint32 NEW_AGENT_RESOURCES_PRICE = 77 ;
const QSize WORLD_SIZE(800, 800);
const qreal WAREHOUSE_INITIAL_RADIUS = 25;
const qreal RESOURCE_INITIAL_RADIUS = 25;

class Agent;

struct AcousticMessage
{
    QMutex minDistanceToResourceAccess;
    qreal minDistanceToResource = -1;
    Agent* minDistanceToResourceSender = nullptr;

    QMutex minDistanceToWarehouseAccess;
    qreal minDistanceToWarehouse = -1;
    Agent* minDistanceToWarehouseSender = nullptr;


    /** For given radius return collection of points with integer coords that reside in circle with this raius and center (0,0)

        For given radius results should be cached
    */
    static const QVector<QPoint>& relativeCoordsCollection(qint32 radius)
    {
        static QReadWriteLock lock;

        static QMap<quint32, QVector<QPoint>> cache;

        lock.lockForRead();
        if (!cache.contains(radius))
        {
            lock.unlock();
            lock.lockForWrite();
            if (!cache.contains(radius))
            {
                for(qint32 x = -radius; x <= radius; x++)
                    for(qint32 y = -radius; y <= radius; y++)
                        if ( x*x + y*y <= radius*radius )
                            cache[radius].append({x,y});
            }
        }
        lock.unlock();
        return cache[radius];
    }
};

class AcousticSpace
{
    AcousticMessage** space = nullptr;
    QRect boundRect;
public:
    AcousticSpace(QRect bound)
        :boundRect(bound)
    {
        space = new AcousticMessage* [bound.width()];
        for(int x=0; x<bound.width(); x++)
            space[x] = new AcousticMessage [bound.height()];
    }

    AcousticMessage& cell(QPointF coord)
    {
        int x_index = coord.x() - boundRect.left();
        int y_index = coord.y() - boundRect.top();
        if (x_index >=0 && y_index >= 0 && x_index < boundRect.width() && y_index<boundRect.height())
            return space[x_index][y_index];
        else
            throw std::range_error("index out of range");
    }

    void clear()
    {
        for(int x=0; x<boundRect.width(); x++)
            for(int y=0; y<boundRect.height(); y++)
            {
                space[x][y].minDistanceToResourceSender = nullptr;
                space[x][y].minDistanceToWarehouseSender = nullptr;
            }
    }

    void shout(const AcousticMessage& msg, QPoint pos, int range)
    {
        auto& relPointsCollection = AcousticMessage::relativeCoordsCollection(range);
        foreach(const QPoint& relPoint, relPointsCollection)
        {
            QPoint acousticCoord = pos + relPoint;
            int x_index = acousticCoord.x() - boundRect.left();
            int y_index = acousticCoord.y() - boundRect.top();
            if (x_index >=0 && y_index >= 0 && x_index < boundRect.width() && y_index<boundRect.height())
            {
                space[x_index][y_index].minDistanceToResourceAccess.lock();
                if (space[x_index][y_index].minDistanceToResource > msg.minDistanceToResource || !space[x_index][y_index].minDistanceToResourceSender)
                {
                    if (space[x_index][y_index].minDistanceToResource > msg.minDistanceToResource || !space[x_index][y_index].minDistanceToResourceSender)
                    {
                        space[x_index][y_index].minDistanceToResource = msg.minDistanceToResource;
                        space[x_index][y_index].minDistanceToResourceSender = msg.minDistanceToResourceSender;
                    }
                }
                space[x_index][y_index].minDistanceToResourceAccess.unlock();

                space[x_index][y_index].minDistanceToWarehouseAccess.lock();
                if (space[x_index][y_index].minDistanceToWarehouse > msg.minDistanceToWarehouse || !space[x_index][y_index].minDistanceToWarehouseSender)
                {
                    if (space[x_index][y_index].minDistanceToWarehouse > msg.minDistanceToWarehouse || !space[x_index][y_index].minDistanceToWarehouseSender)
                    {
                        space[x_index][y_index].minDistanceToWarehouse = msg.minDistanceToWarehouse;
                        space[x_index][y_index].minDistanceToWarehouseSender = msg.minDistanceToWarehouseSender;
                    }
                }
                space[x_index][y_index].minDistanceToWarehouseAccess.unlock();
            }
        }
    }
};


class World : public QObject
{
    Q_OBJECT

    AcousticSpace* acousticSpace = nullptr;

    QSize size;
    QList<PointOfInterest*> pResources;
    QList<PointOfInterest*> pWarehouse;
    //QVector<QVector<Agent*>> agents;
    QVector<Agent*> agents;
    bool stopRequested = false;

    PointOfInterest* generateResource();
    PointOfInterest* generateWarehouse();

    QVector<QPair<const Agent*, const Agent*>> communicatedAgents;
public:
    World(QObject* parent = nullptr);
    void stop();

    QSizeF worldSize() const;
    QRectF boundRect() const;
    QPointF randomWorldCoord(qreal margin) const;
    qreal minXcoord() const;
    qreal maxXcoord() const;
    qreal minYcoord() const;
    qreal maxYcoord() const;

    PointOfInterest* resourceAt(QPointF pos, quint16 r);
    PointOfInterest* warehouseAt(QPointF pos, quint16 r);

    qreal grabResource(PointOfInterest* poi, qreal capacity);
    qreal dropResource(PointOfInterest* poi, qreal volume);

    QMutex commLinesAccess;
    const QVector<QPair<const Agent*, const Agent*>>& commLines() {commLinesAccess.lock(); return communicatedAgents;}
    void  commLinesRelease()  {commLinesAccess.unlock();}

#if QT_VERSION >= 0x051400
    QRecursiveMutex agentListAccess;
#else
    QMutex agentListAccess;
#endif
    //const QVector<Agent*>& agentList() { agentListAccess.lock(); return agents; }
    //void agentListRealease() { agentListAccess.unlock();}
    void forEachAgent(std::function<void(Agent*)> f)
    {
        QMutexLocker lock(&agentListAccess);
        //foreach (QVector<Agent*> cluster, agents)
        {
            foreach(Agent* agent, agents)
                f(agent);
        }
    }

    const QList<PointOfInterest*> warehouseList() const { return pWarehouse; }

    QMutex resourcesAccess;
    void forEachResource(std::function<void(PointOfInterest*)> f)
    {
        QMutexLocker lock(&resourcesAccess);
        foreach(PointOfInterest* poi, pResources)
            f(poi);
    }

private slots:
    void onNewCommunication(Agent*, Agent*);


public slots:
    void onNewResourceRequest();
    void onNewWarehouseRequest();

    Agent *generateNewAgent(QPointF);
    void iteration();

    void onStart();
signals:
    void agentCreated(Agent* a);
    void agentDied(Agent* );
    void resourceDepleted(PointOfInterest* );
    void resourceAppeared(PointOfInterest* );
    void warehouseAppeared(PointOfInterest* );

    void iterationStart();
    void iterationEnd(quint64);

    void requestNewAgent(QPointF);
};
#endif // WORLD_H
