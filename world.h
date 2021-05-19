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
const qreal PI = 3.1415926;

const quint16 AGENTS_COUNT = 400;
const quint16 GRANULARITY_US = 1000;
const quint32 WAREHOUSE_RESOURCES_TO_GENERATE_NEW_AGENTS = 10000;
const quint32 NEW_AGENT_RESOURCES_PRICE = 1000;
const QSize WORLD_SIZE(2000, 1000);

class Agent;

struct Coord
{
    long x;
    long y;
};

struct Cell
{
    Agent* pAgent = nullptr;
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

class World : public QObject
{
    Q_OBJECT

    QVector<QVector<Cell>> cells;

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
