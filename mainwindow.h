#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "src/videoplayer/videoplayer.h"
#include "src/media/VideoPage.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Videoplayer *videoplayer;
    void openFile();


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initView();

private:
    VideoPage *videoPage;
    QStackedLayout *contentLayout;

};
#endif // MAINWINDOW_H

