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
    QVector<QGraphicsItem*> communicationLines;
public:
    MainWindow(World &world, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QGraphicsView* view();
    QGraphicsScene* scene;

    quint16 agentsCreatedCount = 0;

    void createPoiAvatar(PointOfInterest& poi);

    quint32 frameCount = 0;
private slots:
    void onResourceAppeared(PointOfInterest* poi);
    void onWarehouseAppeared(PointOfInterest* poi);
    void onResourceDepleted(PointOfInterest* poi);
    void onAgentCreated(Agent* a);
    void onAgentDied(Agent* a);
    void drawFrame(quint64 calcTime);
    void removeCommunicationLines();

signals:
    void newResourceRequest();
    void readyForNewFrame();
};

#endif // MAINWINDOW_H
