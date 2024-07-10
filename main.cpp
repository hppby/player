

#include "mainwindow.h"
#include "src/home/home.h"
#include "src/media/VideoPage.h"

#include <QApplication>

int main(int argc, char *argv[])
{


    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();


    VideoPage videoPage;
    videoPage.show();

//    Home home;
//    home.show();

    return a.exec();
}
