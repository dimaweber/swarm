#ifndef AGENT_H
#define AGENT_H
#include "world.h"

#include <QBrush>
#include <QPen>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>

class QGraphicsItem;

struct AgentAvatar
{
    QGraphicsItemGroup* grp = nullptr;
    QGraphicsEllipseItem* body = nullptr;
    QGraphicsEllipseItem* aura = nullptr;
    bool valid = true;

    void setPos(QPointF p)
    { if (grp) grp->setPos(p);}

    void setBrush(QBrush b)
    { if (body) body->setBrush(b); }

    void destroy()
    {
        QGraphicsScene* scene = body->scene();
        scene->removeItem(aura);
        scene->removeItem(body);
        scene->removeItem(aura);

        grp = nullptr;
        body = nullptr;
        aura = nullptr;
    }
};

class Agent : public QObject
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

    QPointF position;
    qreal r=1;
    qreal capacity = 1.0;
    qreal carriedResourceVolume = 0;
    qreal shoutRange = 50;
    int ttl = 1000;

    QBrush brushEmpty;
    QBrush brushFull;
    QPen   penEmpty;
    QPen   penFull;
    AgentSpeed speed;
    //State agentState;

    World* pWorld;

    quint16 distanceToResource = 10000;
    quint16 distanceToWarehouse = 10000;

    AgentAvatar  avtr;
public:
    Agent(World* world, QPointF initialPosition = QPointF(), QObject* parent = nullptr);

    QRectF boundRect() const;
    QBrush brush() const;
    QPen pen() const;
    QPointF pos() const;

    AgentAvatar* avatar();
    void buildAvatar(QGraphicsScene* scene);


    State state() const;

    qreal sqDistanceTo(QPointF a, QPointF b);

    void setPos(QPointF p) {position = p;}

    qreal radius() const {return r;}

    void acousticShout(AcousticSpace &space);
    void acousticListen(AcousticSpace &space);
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
