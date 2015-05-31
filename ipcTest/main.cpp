#include "MainWindow.h"
#include <QApplication>
#include "testEnv.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
    testEnv test;
    test.createAndRunThreads();

    return a.exec();
}
