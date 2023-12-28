#include <QPushButton>
#include <QFileDialog>
#include "mainwindow.h"
#include "QMenuBar"
#include "src/home/home.h"
#include "QDebug"
#include "QLabel"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    this->setFixedSize(600,400);

    QLabel *label = new QLabel(this);
    label->setFixedSize(300, 200);
    label->setFrameRect(QRect(100, 100, 400, 100));
    label->setText("大宝专属播放器！！！");
    label->setFont(QFont(":serif", 20, 500));
    label->setAlignment(Qt::AlignCenter);



    QPushButton *searchBtn = new QPushButton(this);

    searchBtn->setText("点击开始");


    searchBtn->setGeometry(250, 300, 100, 50);

//    Home * home = new Home();

   videoplayer = new Videoplayer();
//    connect(searchBtn, &QPushButton::clicked, this, [=]() {
//        qDebug() << "dianjile";
//        this->hide();
//        videoplayer->show();
//    });

    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::openFile);


}

void MainWindow::openFile() {
    //定义文件对话框类
    QFileDialog *fileDialog = new QFileDialog(this);

    //定义文件对话框标题
    fileDialog->setWindowTitle(QStringLiteral("选择文件"));

    //设置打开的文件路径
    fileDialog->setDirectory("~/Downloads/");

    //设置文件过滤器,只显示.ui .cpp 文件,多个过滤文件使用空格隔开
    fileDialog->setNameFilter(tr("File(*)"));

    //设置可以选择多个文件,默认为只能选择一个文件QFileDialog::ExistingFiles
    fileDialog->setFileMode(QFileDialog::ExistingFile);

    //设置视图模式
    fileDialog->setViewMode(QFileDialog::Detail);

    //获取选择的文件的路径
    QStringList fileNames;
    if (fileDialog->exec()) {
        fileNames = fileDialog->selectedFiles();

        qDebug() << "选择的文件是 "<<  fileNames ;

        if (!videoplayer->isActiveWindow()) {
        videoplayer->show();
        }

        videoplayer->openFile(fileNames.first());

        fileDialog->setAttribute(Qt::WA_DeleteOnClose, true);
        fileDialog->destroyed();
        fileDialog=NULL;
        qDebug() << "fileDialog" << fileDialog ;



    }

}



MainWindow::~MainWindow()
{
}


