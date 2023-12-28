//
// Created by LOOG LS on 2023/12/16.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Home.h" resolved

#include <QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include "home.h"
#include "QPushButton"
#include <QWidget>
#include "QLineEdit"
#include <QVideoWidget>
#include <QStackedLayout>


Home::Home(QWidget *parent) :
        QMainWindow(parent){

//this->setFixedSize(1280, 720);



//QHBoxLayout *hLayout = new QHBoxLayout(this);
//
//    QLineEdit *edit = new QLineEdit(this);
//    edit->setPlaceholderText("请输入电影/电视剧名字");
//    edit->setFixedWidth(44);
//
//    QPushButton *searchBtn = new QPushButton(this);
//    searchBtn->setFixedWidth(100);
//    searchBtn->setFixedHeight(44);
//    searchBtn->setText("搜索");
//
//    hLayout->addWidget(edit);
//    hLayout->addWidget(searchBtn);
//
//    hLayout->setGeometry(QRect(0,0,this->width(),100));

    this->setFixedSize(600, 500);


////w2->setStyleSheet( "#widget{background:rgba(255,0,0,0.3);}");
//w2->setOpacity(0.4);
//w2->setAttribute(Qt::WA_TranslucentBackground);
//
//QWidget * w3 =new QWidget(this);
//w3->setGeometry(350,150,100,100);
//w3->setStyleSheet("background-color: green");

    QStackedLayout *stackWidget = new QStackedLayout(this);
//    QVideoWidget * w1 = new QVideoWidget(this);
    QWidget * w1 = new QWidget(this);

    QWidget * w2 =new QWidget(this);
    stackWidget->setGeometry(QRect(0,0, 600,400));
    stackWidget->setStackingMode(QStackedLayout::StackAll);
    stackWidget->addWidget(w1);
    stackWidget->addWidget(w2);
    w1->setGeometry(100,100,100,100);
    w2->setGeometry(150,150,100,100);
    w1->setStyleSheet("background-color: red");
    w2->setObjectName("wid");
    w2->setStyleSheet("#wid{background: rgba(0,255,0,0.5);}");
    stackWidget->setCurrentIndex(1);
//    w2->setStyleSheet("background-color: blue");
//    stackWidget->addWidget(w3);

//    stackWidget->setFixedSize(600, 500);

}

