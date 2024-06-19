

#include "mainwindow.h"
#include "src/home/home.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();


//    Home home;
//    home.show();

    return a.exec();
}
