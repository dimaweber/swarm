#ifndef WORLD_H
#define WORLD_H

#include <QSize>
#include <QPointF>
#include <QGraphicsEllipseItem>
#include <QBrush>
#include <QPen>
#include <QDataStream>
#include <QVector>
#include <QThread>
#include <QMutex>
#include <QRecursiveMutex>
#include <QMap>

const qreal PI = 3.1415926;

const quint16 AGENTS_COUNT = 400;
const quint16 GRANULARITY_US = 1000;
const quint32 WAREHOUSE_RESOURCES_TO_GENERATE_NEW_AGENTS = 10000;
const quint32 NEW_AGENT_RESOURCES_PRICE = 1000;
const QSize WORLD_SIZE(1000, 1000);
const qreal WAREHOUSE_INITIAL_RADIUS = 25;
const qreal RESOURCE_INITIAL_RADIUS = 25;

class Agent;

struct Coord
{
    long x;
    long y;
};

struct AcousticMessage
{
    qreal distanceToResource;
    qreal disatnceToWarehouse;
    Agent* sender;

    /** For given radius return collection of points with integer coords that reside in circle with this raius and center (0,0)

        For given radius results should be cached
    */
    static const QVector<QPoint>& relativeCoordsCollection(qint32 radius)
    {
        static QMap<quint32, QVector<QPoint>> cache;
        if (!cache.contains(radius))
        {
            for(qint32 x = -radius; x <= radius; x++)
                for(qint32 y = -radius; y <= radius; y++)
                    if ( x*x + y*y <= radius*radius )
                    {
                        cache[radius].append({x,y});
                    }
        }
        return cache[radius];
    }
};

class AcousticSpace
{
    QVector<AcousticMessage>** space = nullptr;
    QRect boundRect;
public:
    AcousticSpace(QRect bound)
        :boundRect(bound)
    {
        space = new QVector<AcousticMessage>* [bound.width()];
        for(int x=0; x<bound.width(); x++)
            space[x] = new QVector<AcousticMessage> [bound.height()];
    }

    QVector<AcousticMessage>& cell(QPointF coord)
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
                space[x][y].clear();
    }

    void shout(AcousticMessage msg, QPoint pos, int range)
    {
        auto& relPointsCollection = AcousticMessage::relativeCoordsCollection(range);
        foreach(const QPoint& relPoint, relPointsCollection)
        {
            QPoint acousticCoord = pos + relPoint;
            int x_index = acousticCoord.x() - boundRect.left();
            int y_index = acousticCoord.y() - boundRect.top();
            if (x_index >=0 && y_index >= 0 && x_index < boundRect.width() && y_index<boundRect.height())
                space[x_index][y_index].append(msg);
        }

    }
};

struct Cell
{
    QVector<AcousticMessage> messages;
};

struct PointOfInterest
{
    QPointF pos;
    qreal radius;
    qreal volume;
    qreal criticalVolume;
    QColor color;
    bool valid = true;
    QGraphicsEllipseItem* pAvatar = nullptr;

    QRectF boundRect() const
    {
        return QRectF(-radius, -radius, 2*radius, 2*radius);
    }

    bool collaide(QPointF point, quint16 r)
    {
        return pow(point.x() - pos.x(), 2) + pow(point.y()-pos.y(),2) <= pow(radius+r, 2);
    }
};

bool operator < (const QPoint& a, const QPoint& b);

class World : public QObject
{
    Q_OBJECT

    AcousticSpace* acousticSpace = nullptr;

    QSize size;
    QList<PointOfInterest*> pResources;
    PointOfInterest* pWarehouse = nullptr;
    QVector<Agent*> agents;
    bool stopRequested = false;

    PointOfInterest* generateResource();
    PointOfInterest* generateWarehouse();

    QVector<QPair<const Agent*, const Agent*>> communicatedAgents;
public:
    World(QObject* parent = nullptr);
    void stop();
    Cell* cell(const Coord& ) const;

    QSizeF worldSize() const;
    QRectF boundRect() const;
    QPointF randomWorldCoord(qreal margin) const;
    qreal minXcoord() const;
    qreal maxXcoord() const;
    qreal minYcoord() const;
    qreal maxYcoord() const;

    PointOfInterest* resourceAt(QPointF pos, quint16 r);
    bool isWarehouseAt(QPointF pos, quint16 r);

    qreal grabResource(PointOfInterest* poi, qreal capacity);
    qreal dropResource(qreal volume);

    QMutex commLinesAccess;
    const QVector<QPair<const Agent*, const Agent*>>& commLines() {commLinesAccess.lock(); return communicatedAgents;}
    void  commLinesRelease()  {commLinesAccess.unlock();}

    QRecursiveMutex agentListAccess;
    const QVector<Agent*>& agentList() { agentListAccess.lock(); return agents; }
    void agentListRealease() { agentListAccess.unlock();}

    const PointOfInterest* warehouse() const { return pWarehouse; }

    QMutex resourcesAccess;
    const QList<PointOfInterest*> resourcesList() { resourcesAccess.lock(); return pResources;}
    void resourcesListRelease() {resourcesAccess.unlock();}
private slots:
    void onNewCommunication(Agent*, Agent*);


public slots:
    void onNewResourceRequest();
    void onNewWarehouseRequest();

    Agent *generateNewAgent();
    void iteration();

    void onStart();
signals:
    void agentCreated(Agent* a);
    void resourceDepleted(QAbstractGraphicsShapeItem* );
    void resourceAppeared(PointOfInterest* );
    void warehouseAppeared(PointOfInterest* );

    void iterationStart();
    void iterationEnd(quint64);
};
#endif // WORLD_H
