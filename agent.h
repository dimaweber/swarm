#ifndef AGENT_H
#define AGENT_H
#include "world.h"
#include "poi.h"

#include <QBrush>
#include <QPen>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>

class QGraphicsItem;
class QJsonObject;

struct AgentAvatar
{
    QGraphicsItemGroup* grp = nullptr;
    QGraphicsPolygonItem* direct = nullptr;
    QGraphicsEllipseItem* body = nullptr;
    QGraphicsEllipseItem* aura = nullptr;
    QGraphicsLineItem* head = nullptr;

    bool valid = true;

    void setPos(QPointF p)
    { if (grp) grp->setPos(p);}

    void setBrush(QBrush b)
    {
        //if (body) body->setBrush(b);
        if (direct) direct->setBrush(b);
    }

    void setPen(QPen p)
    {
        if (body) body->setPen(p);
        if (direct) direct->setPen(p);
        if (head) head->setPen(p);
    }

    void destroy()
    {
        QGraphicsScene* scene = body->scene();
        scene->removeItem(aura);
        scene->removeItem(body);
        scene->removeItem(aura);
        scene->removeItem(direct);
        scene->removeItem(head);

        grp = nullptr;
        body = nullptr;
        aura = nullptr;
        direct = nullptr;
        head = nullptr;
    }

    void setRotate(qreal angle)
    {
       if (grp)
            grp->setRotation((angle - PI/2)/ PI * 180);
    }
};

class Agent : public WorldObject
{
    Q_OBJECT
public:
    enum State {Empty, Full, Dead};
private:
    struct AgentSpeed
    {
        qreal dist = 0;
        qreal angle = 0; // radian
    };

    //QPointF position;
//    qreal r = DEFAULT_INITIAL_AGENT_RADIUS;
    //qreal capacity = 1.0;
    //qreal carriedResourceVolume = 0;
    qreal shoutRange = 50;
    int ttl = 1000;

    QColor colorEmpty;
    QColor colorFull;
    AgentSpeed speed;
    //State agentState;

    World* pWorld;

    qreal distanceToResource = 10000;
    qreal distanceToWarehouse = 10000;

    AgentAvatar  avtr;
public:
    Agent(World* world, QPointF initialPosition = QPointF(), QObject* parent = nullptr);

    AgentAvatar* avatar();
    void buildAvatar(QGraphicsScene* scene);


    State state() const;

    qreal direction() const {return speed.angle;}

    void acousticShout(AcousticSpace &space);
    void acousticListen(AcousticSpace &space);

    QColor color() const;

    void read (const QJsonObject& json);
    void write (QJsonObject& json) const;

public slots:
    void move();

private slots:
    void setInitialSpeed();
    void setInitialCoord();

signals:
    void movedTo(QPointF);
    void movedFrom(QPointF);

    void anounceDistances();
    void newCommunication(Agent*, Agent*);
};

#endif // AGENT_H
