#include "mainwindow.h"
#include "world.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "swarm_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    World world;
    QThread worldThread;
    world.moveToThread( &worldThread);
    worldThread.connect (&worldThread, &QThread::started, &world, &World::onStart);

    MainWindow w(world);
    w.show();
    worldThread.start();
    a.exec();

    worldThread.wait();
}
