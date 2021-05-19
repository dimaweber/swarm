#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QGraphicsView>
#include "agent.h"

#include <QGraphicsItem>
#include <QDataStream>
#include <QFile>
#include <QElapsedTimer>


MainWindow::MainWindow(World &world, QWidget *parent)
    : QDialog(parent),
      world(world),
      ui(new Ui::MainWindow),
      scene (new QGraphicsScene(world.boundRect()))
{
    ui->setupUi(this);

    ui->agentsCountLabel->setNum(0);
    ui->emptyAgentsCount->setNum(0);
    ui->fullAgentsCount->setNum(0);

    ui->graphicsView->setMinimumSize(world.worldSize().toSize() + QSize(30,30));

    setLayout(ui->horizontalLayout);

    QRectF worldBorder = world.boundRect();

//    scene->setSceneRect(worldBorder);
    scene->addRect(worldBorder);

    scene->addEllipse(-502, -502, 5, 5, {"red"}, QBrush("blue"));
    scene->addEllipse( 498,  498, 5, 5, {"red"}, {"blue"});
    scene->addEllipse(-502,  498, 5, 5, QPen("red"), QBrush("blue"));
    scene->addEllipse( 498,  -502, 5, 5, QPen("red"), QBrush("blue"));

    connect (&world, &World::agentCreated, this, &MainWindow::onAgentCreated);
    connect (&world, &World::resourceAppeared, this, &MainWindow::onResourceAppeared);
    connect (&world, &World::warehouseAppeared, this, &MainWindow::onWarehouseAppeared);
    connect (&world, &World::resourceDepleted, this, &MainWindow::onResourceDepleted);
    connect (&world, &World::iterationEnd, this, &MainWindow::drawFrame);
    connect (&world, &World::iterationStart, this, &MainWindow::removeCommunicationLines);
    connect (this, &MainWindow::readyForNewFrame, &world, &World::iteration);
    ui->graphicsView->setScene(scene);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QGraphicsView* MainWindow::view()
{
    return  ui->graphicsView;
}

QAbstractGraphicsShapeItem* MainWindow::createPoiAvatar(PointOfInterest& poi)
{
    QBrush brush(poi.color);
    QPen pen;
    pen.setWidth(2);


    QGraphicsEllipseItem* gItem = scene->addEllipse(poi.boundRect(), pen, brush);
    gItem->setPos(poi.pos);

    poi.pAvatar = gItem;
    return gItem;
}

void MainWindow::onResourceAppeared(PointOfInterest* poi)
{
    createPoiAvatar(*poi);
}

void MainWindow::onWarehouseAppeared(PointOfInterest* poi)
{
    createPoiAvatar(*poi);
    QBrush b = poi->pAvatar->brush();
    QColor c = b.color();
    c.setAlpha( 0);
    b.setColor(c);
    poi->pAvatar->setBrush(b);
}

void MainWindow::onResourceDepleted(QAbstractGraphicsShapeItem* avatar)
{
    if (avatar)
        scene->removeItem(avatar);
}

void MainWindow::onAgentCreated(Agent *a)
{
    a->buildAvatar(scene);

    if (a->state() == Agent::Empty)
        emptyAgentsCount++;
    else
        fullAgentsCount++;
    agentsCreatedCount++;
    ui->emptyAgentsCount->setNum(emptyAgentsCount);
    ui->fullAgentsCount->setNum(fullAgentsCount);
    ui->agentsCountLabel->setNum(agentsCreatedCount);
}

void MainWindow::drawFrame(quint64 calcTime)
{
    QElapsedTimer renderTimer;
    renderTimer.start();
    auto& agents = world.agentList();
    emptyAgentsCount = 0;
    fullAgentsCount = 0;
    foreach(Agent* agent, agents)
    {
        agent->avatar()->setPos(agent->pos());

        agent->avatar()->setBrush(agent->brush());
        if (agent->state() == Agent::Empty)
        {
            ++emptyAgentsCount;
            //--fullAgentsCount;
        }
        else
        {
            //--emptyAgentsCount;
            ++fullAgentsCount;
        }
    }
    world.agentListRealease();
    ui->emptyAgentsCount->setNum(emptyAgentsCount);
    ui->fullAgentsCount->setNum(fullAgentsCount);

    auto& comm = world.commLines();
    QPen redPen(QColor("red"));
    QPen greenPen(QColor("green"));
    redPen.setWidth(2);
    greenPen.setWidth(2);

    foreach (auto& l, comm)
    {
        QLineF line(l.first->pos(), l.second->pos());
        communicationLines.append(scene->addLine(line, l.first->state()==Agent::Empty?redPen:greenPen));
    }
    world.commLinesRelease();

    world.warehouse()->pAvatar->setRect(world.warehouse()->boundRect());
    if (world.warehouse()->volume < world.warehouse()->criticalVolume)
    {
        QPen p = world.warehouse()->pAvatar->pen();
        p.setWidth(1);
        QBrush b = world.warehouse()->pAvatar->brush();
        QColor c = b.color();
        c.setAlpha( world.warehouse()->volume / world.warehouse()->criticalVolume * 255);
        b.setColor(c);
        world.warehouse()->pAvatar->setBrush(b);
        world.warehouse()->pAvatar->setPen(p);
    }
    else
    {
        QPen p = world.warehouse()->pAvatar->pen();
        p.setWidth(3);

        world.warehouse()->pAvatar->setPen(p);
    }


    foreach(PointOfInterest* resource, world.resourcesList())
    {
        resource->pAvatar->setRect(resource->boundRect());
    }
    world.resourcesListRelease();

    ui->warehouseVolumeLabel->setNum(world.warehouse()->volume);

    emit readyForNewFrame();

    ui->calcTimeLabel->setNum((double)calcTime);
    ui->renderTimeLabel->setNum((double)renderTimer.elapsed());
    ui->fpsLabel->setNum((double)++frameCount);
}

void MainWindow::removeCommunicationLines()
{
    foreach(auto l, communicationLines)
    {
        scene->removeItem(l);
    }
    communicationLines.clear();
}
