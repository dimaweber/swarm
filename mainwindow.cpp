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

void MainWindow::createPoiAvatar(PointOfInterest& poi)
{
    poi.buildAvatar(scene);
}

void MainWindow::onResourceAppeared(PointOfInterest* poi)
{
    createPoiAvatar(*poi);
}

void MainWindow::onWarehouseAppeared(PointOfInterest* poi)
{
    createPoiAvatar(*poi);
    poi->avatar()->setAlpha(0);
}

void MainWindow::onResourceDepleted(PointOfInterest *poi)
{
    if (poi)
        poi->avatar()->destroy();
}

void MainWindow::onAgentCreated(Agent *a)
{
    a->buildAvatar(scene);

    agentsCreatedCount++;
    ui->agentsCountLabel->setNum(agentsCreatedCount);
}

void MainWindow::drawFrame(quint64 calcTime)
{
    QElapsedTimer renderTimer;
    renderTimer.start();
    quint16 fullAgentsCount = 0;
    quint16 emptyAgentsCount = 0;
    world.forEachAgent([&fullAgentsCount, &emptyAgentsCount](Agent* agent)
    {
        agent->avatar()->setPos(agent->pos());

        agent->avatar()->setBrush(agent->brush());
        if (agent->state() == Agent::Empty)
            ++emptyAgentsCount;
        else
            ++fullAgentsCount;
    });

    ui->emptyAgentsCount->setNum(emptyAgentsCount);
    ui->fullAgentsCount->setNum(fullAgentsCount);

    QColor redColor("red");
    QColor greenColor("green");
    redColor.setAlphaF(0.2);
    greenColor.setAlphaF(0.2);
    QPen redPen(redColor);
    QPen greenPen(greenColor);
    redPen.setWidth(2);
    greenPen.setWidth(2);

    if (ui->showCommunicationLinesCheckbox->isChecked())
    {
        auto& comm = world.commLines();
        foreach (auto& l, comm)
        {
            QLineF line(l.first->pos(), l.second->pos());
            communicationLines.append(scene->addLine(line, l.first->state()==Agent::Empty?redPen:greenPen));
        }
        world.commLinesRelease();
    }

    foreach (PointOfInterest* warehouse, world.warehouseList())
    {
        auto pAvatar = warehouse->avatar();
        pAvatar->setRect(warehouse->boundRect());
        if (warehouse->volume() < warehouse->criticalVolume)
        {
            pAvatar->setBorderWidth(1);
            pAvatar->setAlpha( warehouse->volume() / warehouse->criticalVolume * 255);
        }
        else
        {
            pAvatar->setBorderWidth(3);
        }
    }

    world.forEachResource([](PointOfInterest* resource)
    {
        resource->avatar()->setRect(resource->boundRect());
    });

    quint32 volSum = 0;
    foreach (PointOfInterest* warehouse, world.warehouseList()) {
        volSum += warehouse->volume();
    }
    ui->warehouseVolumeLabel->setNum((double)volSum);

    emit readyForNewFrame();

    ui->calcTimeLabel->setNum((double)calcTime);
    ui->renderTimeLabel->setNum((double)renderTimer.elapsed());
    ui->fpsLabel->setNum((double)++frameCount);
}

void MainWindow::removeCommunicationLines()
{
    foreach(auto l, communicationLines)
        scene->removeItem(l);

    communicationLines.clear();
}
