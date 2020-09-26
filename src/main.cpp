#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QStringList args = a.arguments();
    // filename is args.at(1), if present
    MainWindow w(args.count() < 2 ? "" : args.at(1));
    w.show();

    return a.exec();
}
