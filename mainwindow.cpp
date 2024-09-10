#include <QPushButton>
#include <QFileDialog>
#include "mainwindow.h"
#include "QMenuBar"
#include "src/home/home.h"
#include "QDebug"
#include "QLabel"
#include "src/components/Button.h"
#include <QListView>
#include <QStringListModel>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{



    this->setFixedSize(600,400);

    initView();

//    QLabel *label = new QLabel(this);
//    label->setFixedSize(300, 200);
//    label->setFrameRect(QRect(100, 100, 400, 100));
////    label->setText("大宝专属播放器！！！");
//    label->setFont(QFont(":serif", 20, 500));
//    label->setAlignment(Qt::AlignCenter);
//
//
//
//    QPushButton *searchBtn = new QPushButton(this);
//
//    searchBtn->setText("点击开始");
//
//
//    searchBtn->setGeometry(250, 300, 100, 50);
//
////    Home * home = new Home();
//
//   videoplayer = new Videoplayer();
////    connect(searchBtn, &QPushButton::clicked, this, [=]() {
////        qDebug() << "dianjile";
////        this->hide();
////        videoplayer->show();
////    });
//
//    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::openFile);


}

void MainWindow::initView() {
    // 创建左侧的目录列表
    QWidget *menuView = new QWidget(this);
    menuView->setStyleSheet("background-color: #f0f0f0;");
    QVBoxLayout *menuLayout = new QVBoxLayout(menuView);

    Button *btn1 = new Button("本地", this);
    Button *btn2 = new Button("在线", this);


    menuLayout->addWidget(btn1);
    menuLayout->addWidget(btn2);
    menuLayout->addStretch(1); // 添加伸缩空间
    menuView->setLayout(menuLayout);
    menuView->setFixedWidth(100);

    // 创建右侧的主页面
    QWidget *contentWidget = new QWidget(this);
    contentWidget->setMinimumWidth(400); // 设置最小宽度

    QStackedLayout *contentLayout = new QStackedLayout(contentWidget);
    videoplayer = new Videoplayer(contentWidget);
    contentLayout->addWidget(videoplayer);
    videoPage = new VideoPage(contentWidget);
    contentLayout->addWidget(videoPage);



    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(menuView);
    mainLayout->addWidget(contentWidget);

    // 设置 mainLayout 为主布局
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    connect(btn1, &Button::clicked, this, [=]{
        if (contentLayout->currentIndex() != 0) {
            contentLayout->setCurrentIndex(0);
        }
    });
    connect(btn2, &Button::clicked, this, [=]{
        if (contentLayout->currentIndex() != 1) {
        contentLayout->setCurrentIndex(1);
        }
    });

    // 调整初始大小
    resize(this->width(), this->height());
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


        fileDialog->setAttribute(Qt::WA_DeleteOnClose, true);
        fileDialog->destroyed();
        fileDialog=NULL;
        qDebug() << "fileDialog" << fileDialog ;

        videoplayer->openFile(fileNames.first());

    }

}



MainWindow::~MainWindow() {
}


