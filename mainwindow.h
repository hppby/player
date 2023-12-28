#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "src/videoplayer/videoplayer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Videoplayer *videoplayer;
    void openFile();


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};
#endif // MAINWINDOW_H

