#ifndef POI_H
#define POI_H

#include <QBrush>
#include <QPen>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QThread>

#include <QReadWriteLock>

const qreal PI = 3.1415926;


struct PointOfInterestAvatar
{
    QGraphicsEllipseItem* pEllipse = nullptr;

    void setBrush(QBrush b)
    {
        pEllipse->setBrush(b);
    }

    QBrush brush() const { return pEllipse->brush(); }

    void setPos(QPointF p)
    {
        pEllipse->setPos(p);
    }

    void setAlpha(qreal a)
    {
        QBrush b = brush();
        QColor c = b.color();
        c.setAlpha(a);
        b.setColor(c);
        setBrush(b);
    }

    void setRect(QRectF r)
    {
        pEllipse->setRect(r);
    }

    void setBorderWidth(int w)
    {
        QPen pen = pEllipse->pen();
        pen.setWidth(w);
        pEllipse->setPen(pen);
    }

    void destroy()
    {
        QGraphicsScene* scene = pEllipse->scene();
        if (scene->thread() != QThread::currentThread())
            throw "wrong thread";
        scene->removeItem(pEllipse);
    }
};

class WorldObject : public QObject
{
    Q_OBJECT
    bool valid = true;
    mutable QReadWriteLock accessMutex;
    PointOfInterestAvatar avtr;
    qreal   _volume;
    qreal   _radius;
    qreal   _capacity;
    QPointF _position;
    QColor  _color;

public:
    WorldObject(QObject* parent = nullptr);

    virtual QRectF boundRect() const;

    bool collaide(QPointF point, quint16 r);

    qreal incVolume(qreal v);
    qreal decVolume(qreal v);
    bool tryDecVolume(qreal v);

    virtual void buildAvatar(QGraphicsScene* scene);
    PointOfInterestAvatar* avatar();

    virtual void write(QJsonObject& json) const;
    virtual void read(const QJsonObject&);

    qreal sqDistanceTo(QPointF a);

    bool isValid() const {return valid;}
    qreal radius() const;
    qreal volume() const;
    qreal capacity() const;
    QPointF pos() const;
    virtual QColor color() const;

    void invalidate() { valid = false; }
    WorldObject& setPos(QPointF p);
    WorldObject& setRadius(qreal radius);
    WorldObject& setVolume(qreal v);
    WorldObject& setCapacity(qreal capacity = -1);
    WorldObject& setColor(const QColor& color);
};

#endif // POI_H
