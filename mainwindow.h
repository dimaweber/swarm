#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "world.h"
#include "agent.h"

#include <QDialog>
#include <QVector>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QGraphicsView;
class QGraphicsScene;
class Agent;

class MainWindow : public QDialog
{
    Q_OBJECT

    World& world;
    QVector<QPair<QGraphicsItem*, int>> communicationLines;
public:
    MainWindow(World &world, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QGraphicsView* view();
    QGraphicsScene* scene;

    quint16 agentsCreatedCount = 0;

    void createPoiAvatar(WorldObject& poi);

    quint32 frameCount = 0;
private slots:
    void onResourceAppeared(WorldObject* poi);
    void onWarehouseAppeared(WorldObject* poi);
    void onResourceDepleted(WorldObject* poi);
    void onAgentCreated(Agent* a);
    void onAgentDied(Agent* a);
    void drawFrame(qint64 calcTime);
    void removeCommunicationLines();

    bool save();
signals:
    void newResourceRequest();
    void readyForNewFrame();
};

#endif // MAINWINDOW_H
